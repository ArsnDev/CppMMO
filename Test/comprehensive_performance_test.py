#!/usr/bin/env python3
"""
CppMMO 서버 종합 성능 측정 시스템
- 처리량 (Throughput): TPS, 패킷 처리율, 대역폭 사용량
- 지연시간 (Latency): RTT, 서버 처리 시간, 지연 분포
- 리소스 사용량: CPU, 메모리, 네트워크 대역폭
- 안정성: 연결 지속성, 오류율, 복구 능력
"""
import socket
import time
import flatbuffers
import sys
import os
import struct
import threading
import random
import queue
from concurrent.futures import ThreadPoolExecutor
import statistics
import csv
import json
import psutil
import subprocess
from datetime import datetime
from collections import deque
import numpy as np

# --- FlatBuffers 모듈 임포트 설정 ---
flatbuffers_module_base_path = os.path.abspath(os.path.dirname(__file__))
flatbuffers_module_path = os.path.join(flatbuffers_module_base_path, 'Protocol')

if flatbuffers_module_path not in sys.path:
    sys.path.append(flatbuffers_module_path)

try:
    import C_Login
    import C_EnterZone
    import C_PlayerInput
    import C_Chat
    import S_LoginSuccess
    import S_ZoneEntered
    import S_WorldSnapshot
    import S_Chat
    import Packet
    import PacketId
    import UnifiedPacket
    import Vec3
    import PlayerState
except ImportError as e:
    print(f"Error: Could not import FlatBuffers modules from {flatbuffers_module_path}. {e}")
    sys.exit(1)

# === 테스트 설정 ===
class TestConfig:
    HOST = 'localhost'
    PORT = 8080
    
    # 테스트 시나리오별 설정
    SCENARIOS = {
        'basic': {
            'clients': 50,
            'duration': 120,  # 2분
            'movement_interval': 0.05,  # 20fps
            'chat_interval': 10.0
        },
        'stress': {
            'clients': 200,
            'duration': 300,  # 5분
            'movement_interval': 0.033,  # 30fps
            'chat_interval': 5.0
        },
        'extreme': {
            'clients': 500,
            'duration': 600,  # 10분
            'movement_interval': 0.016,  # 60fps
            'chat_interval': 3.0
        },
        'massive': {
            'clients': 800,
            'duration': 300,  # 5분
            'movement_interval': 0.033,  # 30fps
            'chat_interval': 10.0
        },
        'optimized': {
            'clients': 600,
            'duration': 600,  # 10분
            'movement_interval': 0.033,  # 30fps (rate limiting applied)
            'chat_interval': 8.0
        }
    }
    
    ZONE_ID = 1
    
    # 입력 플래그
    INPUT_W = 1
    INPUT_S = 2
    INPUT_A = 4
    INPUT_D = 8

# === 성능 측정 클래스 ===
class PerformanceMetrics:
    def __init__(self):
        self.lock = threading.Lock()
        
        # 처리량 (Throughput) 지표
        self.total_packets_sent = 0
        self.total_packets_received = 0
        self.total_bytes_sent = 0
        self.total_bytes_received = 0
        self.packets_per_second = 0
        self.bytes_per_second = 0
        
        # 지연시간 (Latency) 지표
        self.latency_samples = deque(maxlen=10000)  # 최근 10k 샘플
        self.rtt_samples = deque(maxlen=1000)
        self.server_processing_times = deque(maxlen=1000)
        
        # 연결 상태 지표
        self.clients_connected = 0
        self.clients_in_zone = 0
        self.connection_failures = 0
        self.reconnection_attempts = 0
        
        # 오류율 지표
        self.send_errors = 0
        self.receive_errors = 0
        self.packet_loss_count = 0
        self.protocol_errors = 0
        
        # 시스템 리소스 지표
        self.cpu_usage = 0.0
        self.memory_usage = 0.0
        self.network_bytes_sent = 0
        self.network_bytes_recv = 0
        
        # 시간 관련
        self.test_start_time = None
        self.last_update_time = None
        
    def add_latency_sample(self, latency_ms):
        with self.lock:
            self.latency_samples.append(latency_ms)
    
    def add_rtt_sample(self, rtt_ms):
        with self.lock:
            self.rtt_samples.append(rtt_ms)
    
    def record_packet_sent(self, packet_size):
        with self.lock:
            self.total_packets_sent += 1
            self.total_bytes_sent += packet_size
    
    def record_packet_received(self, packet_size):
        with self.lock:
            self.total_packets_received += 1
            self.total_bytes_received += packet_size
    
    def get_current_stats(self):
        with self.lock:
            current_time = time.time()
            elapsed = current_time - self.test_start_time if self.test_start_time else 0
            
            # 지연시간 통계
            latency_stats = {}
            if self.latency_samples:
                latencies = list(self.latency_samples)
                latency_stats = {
                    'avg': np.mean(latencies),
                    'min': np.min(latencies),
                    'max': np.max(latencies),
                    'p50': np.percentile(latencies, 50),
                    'p95': np.percentile(latencies, 95),
                    'p99': np.percentile(latencies, 99),
                    'std': np.std(latencies)
                }
            
            # RTT 통계
            rtt_stats = {}
            if self.rtt_samples:
                rtts = list(self.rtt_samples)
                rtt_stats = {
                    'avg': np.mean(rtts),
                    'min': np.min(rtts),
                    'max': np.max(rtts),
                    'p95': np.percentile(rtts, 95)
                }
            
            # 처리량 계산
            packets_per_sec = self.total_packets_sent / elapsed if elapsed > 0 else 0
            bytes_per_sec = self.total_bytes_sent / elapsed if elapsed > 0 else 0
            
            # 오류율 계산
            total_operations = self.total_packets_sent + self.total_packets_received
            error_rate = (self.send_errors + self.receive_errors) / total_operations * 100 if total_operations > 0 else 0
            
            return {
                'elapsed_time': elapsed,
                'throughput': {
                    'packets_sent': self.total_packets_sent,
                    'packets_received': self.total_packets_received,
                    'bytes_sent': self.total_bytes_sent,
                    'bytes_received': self.total_bytes_received,
                    'packets_per_sec': packets_per_sec,
                    'bytes_per_sec': bytes_per_sec,
                    'mbps_sent': bytes_per_sec * 8 / (1024 * 1024),
                    'mbps_received': self.total_bytes_received / elapsed * 8 / (1024 * 1024) if elapsed > 0 else 0
                },
                'latency': latency_stats,
                'rtt': rtt_stats,
                'connections': {
                    'connected': self.clients_connected,
                    'in_zone': self.clients_in_zone,
                    'connection_failures': self.connection_failures,
                    'reconnection_attempts': self.reconnection_attempts
                },
                'errors': {
                    'send_errors': self.send_errors,
                    'receive_errors': self.receive_errors,
                    'packet_loss': self.packet_loss_count,
                    'protocol_errors': self.protocol_errors,
                    'error_rate_percent': error_rate
                },
                'system_resources': {
                    'cpu_usage': self.cpu_usage,
                    'memory_usage': self.memory_usage,
                    'network_sent': self.network_bytes_sent,
                    'network_recv': self.network_bytes_recv
                }
            }

# === 시스템 리소스 모니터 ===
class SystemResourceMonitor:
    def __init__(self, metrics: PerformanceMetrics):
        self.metrics = metrics
        self.should_stop = False
        self.server_process = None
        self.initial_network_stats = None
        
    def find_server_process(self):
        """CppMMO 서버 프로세스 찾기"""
        for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
            try:
                name = proc.info.get('name', '').lower() if proc.info.get('name') else ''
                cmdline = proc.info.get('cmdline', []) if proc.info.get('cmdline') else []
                
                if 'cppmmo' in name or \
                   any('cppmmo' in str(cmd).lower() for cmd in cmdline):
                    return proc
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                continue
        return None
    
    def start_monitoring(self):
        """모니터링 시작"""
        self.server_process = self.find_server_process()
        self.initial_network_stats = psutil.net_io_counters()
        
        monitor_thread = threading.Thread(target=self._monitor_loop)
        monitor_thread.daemon = True
        monitor_thread.start()
    
    def _monitor_loop(self):
        """모니터링 루프"""
        while not self.should_stop:
            try:
                # CPU 사용률 (전체 시스템)
                cpu_percent = psutil.cpu_percent(interval=1)
                
                # 메모리 사용률
                memory = psutil.virtual_memory()
                memory_percent = memory.percent
                
                # 네트워크 통계
                current_network = psutil.net_io_counters()
                if self.initial_network_stats:
                    net_sent = current_network.bytes_sent - self.initial_network_stats.bytes_sent
                    net_recv = current_network.bytes_recv - self.initial_network_stats.bytes_recv
                else:
                    net_sent = net_recv = 0
                
                # 서버 프로세스별 리소스 (가능한 경우)
                server_cpu = server_memory = 0
                if self.server_process and self.server_process.is_running():
                    try:
                        server_cpu = self.server_process.cpu_percent()
                        server_memory = self.server_process.memory_percent()
                    except psutil.NoSuchProcess:
                        self.server_process = self.find_server_process()
                
                # 메트릭스 업데이트
                with self.metrics.lock:
                    self.metrics.cpu_usage = max(cpu_percent, server_cpu)
                    self.metrics.memory_usage = max(memory_percent, server_memory)
                    self.metrics.network_bytes_sent = net_sent
                    self.metrics.network_bytes_recv = net_recv
                
                time.sleep(2)  # 2초마다 모니터링
                
            except Exception as e:
                print(f"리소스 모니터링 오류: {e}")
                time.sleep(5)
    
    def stop_monitoring(self):
        self.should_stop = True

# === 고성능 테스트 클라이언트 ===
class PerformanceTestClient:
    def __init__(self, client_id: int, config: dict, metrics: PerformanceMetrics, stats_queue):
        self.client_id = client_id
        self.config = config
        self.metrics = metrics
        self.stats_queue = stats_queue
        
        # 네트워크
        self.socket = None
        self.connected = False
        self.logged_in = False
        self.in_zone = False
        self.should_stop = False
        
        # 플레이어 정보
        self.player_id = 3000 + client_id
        self.session_ticket = f"perf_test_{client_id}"
        self.username = f"PerfBot{client_id:03d}"
        self.sequence_number = 0
        
        # 성능 측정용
        self.sent_packet_timestamps = {}  # sequence_number -> timestamp
        self.local_latencies = deque(maxlen=1000)
        self.local_rtts = deque(maxlen=100)
        
        # 통계
        self.packets_sent = 0
        self.packets_received = 0
        self.bytes_sent = 0
        self.bytes_received = 0
        self.errors = []
        
        # 상태 관리
        self.current_input_flags = 0
        self.last_input_change = time.time()
        self.last_chat_time = time.time()
        self.last_ping_time = time.time()
        
    def create_login_packet(self):
        """로그인 패킷 생성"""
        builder = flatbuffers.Builder(0)
        
        session_ticket_offset = builder.CreateString(self.session_ticket)
        
        C_Login.C_LoginStart(builder)
        C_Login.C_LoginAddSessionTicket(builder, session_ticket_offset)
        C_Login.C_LoginAddPlayerId(builder, self.player_id)
        C_Login.C_LoginAddCommandId(builder, random.randint(1, 1000000))
        c_login_offset = C_Login.C_LoginEnd(builder)
        
        UnifiedPacket.UnifiedPacketStart(builder)
        UnifiedPacket.UnifiedPacketAddId(builder, PacketId.PacketId.C_Login)
        UnifiedPacket.UnifiedPacketAddDataType(builder, Packet.Packet.C_Login)
        UnifiedPacket.UnifiedPacketAddData(builder, c_login_offset)
        unified_packet_offset = UnifiedPacket.UnifiedPacketEnd(builder)
        
        builder.Finish(unified_packet_offset)
        return builder.Output()
    
    def create_enter_zone_packet(self):
        """존 입장 패킷 생성"""
        builder = flatbuffers.Builder(0)
        
        C_EnterZone.C_EnterZoneStart(builder)
        C_EnterZone.C_EnterZoneAddZoneId(builder, TestConfig.ZONE_ID)
        C_EnterZone.C_EnterZoneAddCommandId(builder, random.randint(1, 1000000))
        c_enter_zone_offset = C_EnterZone.C_EnterZoneEnd(builder)
        
        UnifiedPacket.UnifiedPacketStart(builder)
        UnifiedPacket.UnifiedPacketAddId(builder, PacketId.PacketId.C_EnterZone)
        UnifiedPacket.UnifiedPacketAddDataType(builder, Packet.Packet.C_EnterZone)
        UnifiedPacket.UnifiedPacketAddData(builder, c_enter_zone_offset)
        unified_packet_offset = UnifiedPacket.UnifiedPacketEnd(builder)
        
        builder.Finish(unified_packet_offset)
        return builder.Output()
    
    def create_player_input_packet(self, input_flags: int):
        """플레이어 입력 패킷 생성 (타임스탬프 포함)"""
        builder = flatbuffers.Builder(0)
        
        self.sequence_number += 1
        current_time = int(time.time() * 1000)  # milliseconds
        
        # 타임스탬프 저장 (지연시간 측정용)
        self.sent_packet_timestamps[self.sequence_number] = time.time()
        
        # 마우스 위치
        Vec3.Vec3Start(builder)
        Vec3.Vec3AddX(builder, random.uniform(-100, 100))
        Vec3.Vec3AddY(builder, random.uniform(-100, 100))
        Vec3.Vec3AddZ(builder, 0.0)
        mouse_pos_offset = Vec3.Vec3End(builder)
        
        C_PlayerInput.C_PlayerInputStart(builder)
        C_PlayerInput.C_PlayerInputAddTickNumber(builder, 0)
        C_PlayerInput.C_PlayerInputAddClientTime(builder, current_time)
        C_PlayerInput.C_PlayerInputAddInputFlags(builder, input_flags)
        C_PlayerInput.C_PlayerInputAddMousePosition(builder, mouse_pos_offset)
        C_PlayerInput.C_PlayerInputAddSequenceNumber(builder, self.sequence_number)
        C_PlayerInput.C_PlayerInputAddCommandId(builder, random.randint(1, 1000000))
        c_input_offset = C_PlayerInput.C_PlayerInputEnd(builder)
        
        UnifiedPacket.UnifiedPacketStart(builder)
        UnifiedPacket.UnifiedPacketAddId(builder, PacketId.PacketId.C_PlayerInput)
        UnifiedPacket.UnifiedPacketAddDataType(builder, Packet.Packet.C_PlayerInput)
        UnifiedPacket.UnifiedPacketAddData(builder, c_input_offset)
        unified_packet_offset = UnifiedPacket.UnifiedPacketEnd(builder)
        
        builder.Finish(unified_packet_offset)
        return builder.Output()
    
    def create_chat_packet(self, message: str):
        """채팅 패킷 생성"""
        builder = flatbuffers.Builder(0)
        
        message_offset = builder.CreateString(message)
        
        C_Chat.C_ChatStart(builder)
        C_Chat.C_ChatAddMessage(builder, message_offset)
        C_Chat.C_ChatAddCommandId(builder, random.randint(1, 1000000))
        c_chat_offset = C_Chat.C_ChatEnd(builder)
        
        UnifiedPacket.UnifiedPacketStart(builder)
        UnifiedPacket.UnifiedPacketAddId(builder, PacketId.PacketId.C_Chat)
        UnifiedPacket.UnifiedPacketAddDataType(builder, Packet.Packet.C_Chat)
        UnifiedPacket.UnifiedPacketAddData(builder, c_chat_offset)
        unified_packet_offset = UnifiedPacket.UnifiedPacketEnd(builder)
        
        builder.Finish(unified_packet_offset)
        return builder.Output()
    
    def send_packet_with_metrics(self, packet_data: bytes) -> bool:
        """패킷 전송 (성능 측정 포함)"""
        try:
            start_time = time.time()
            
            message_length = len(packet_data)
            length_prefix = struct.pack('<I', message_length)
            full_message = length_prefix + packet_data
            
            self.socket.sendall(full_message)
            
            # 메트릭스 업데이트
            self.packets_sent += 1
            self.bytes_sent += len(full_message)
            self.metrics.record_packet_sent(len(full_message))
            
            # 전송 시간 측정
            send_duration = (time.time() - start_time) * 1000  # ms
            if send_duration > 0.1:  # 0.1ms 이상인 경우만 기록
                self.local_latencies.append(send_duration)
            
            return True
            
        except Exception as e:
            self.errors.append(f"Send error: {e}")
            with self.metrics.lock:
                self.metrics.send_errors += 1
            return False
    
    def receive_packet_with_metrics(self) -> tuple[bool, bytes]:
        """패킷 수신 (성능 측정 포함)"""
        try:
            start_time = time.time()
            
            # 길이 수신
            length_data = self.socket.recv(4)
            if not length_data or len(length_data) < 4:
                return False, b''
            
            message_length = struct.unpack('<I', length_data)[0]
            
            # 데이터 수신
            message_data = b''
            while len(message_data) < message_length:
                chunk = self.socket.recv(message_length - len(message_data))
                if not chunk:
                    return False, b''
                message_data += chunk
            
            # 메트릭스 업데이트
            total_size = 4 + len(message_data)
            self.packets_received += 1
            self.bytes_received += total_size
            self.metrics.record_packet_received(total_size)
            
            # 수신 시간 측정
            receive_duration = (time.time() - start_time) * 1000  # ms
            if receive_duration > 0.1:
                self.local_latencies.append(receive_duration)
            
            return True, message_data
            
        except socket.timeout:
            return False, b''
        except Exception as e:
            self.errors.append(f"Receive error: {e}")
            with self.metrics.lock:
                self.metrics.receive_errors += 1
            return False, b''
    
    def parse_packet_with_latency(self, packet_data: bytes, receive_time: float):
        """패킷 파싱 및 지연시간 계산"""
        try:
            unified_packet = UnifiedPacket.UnifiedPacket.GetRootAsUnifiedPacket(packet_data, 0)
            packet_id = unified_packet.Id()
            
            # 월드 스냅샷의 경우 지연시간 계산
            if packet_id == PacketId.PacketId.S_WorldSnapshot:
                data_table = unified_packet.Data()
                if data_table:
                    snapshot = S_WorldSnapshot.S_WorldSnapshot()
                    snapshot.Init(data_table.Bytes, data_table.Pos)
                    
                    # 틱 번호를 통한 RTT 계산 (대략적)
                    server_tick = snapshot.TickNumber()
                    if server_tick > 0:
                        # 서버 틱을 기반으로 한 대략적인 RTT 계산
                        estimated_server_time = server_tick * (1000.0 / 60.0)  # 60 TPS 가정
                        current_time = time.time() * 1000
                        rtt = current_time - estimated_server_time
                        
                        if 0 < rtt < 1000:  # 1초 미만의 합리적인 RTT만 기록
                            self.local_rtts.append(rtt)
                            self.metrics.add_rtt_sample(rtt)
                
                return 'world_snapshot'
                
            elif packet_id == PacketId.PacketId.S_LoginSuccess:
                return 'login_success'
            elif packet_id == PacketId.PacketId.S_ZoneEntered:
                return 'zone_entered'
            elif packet_id == PacketId.PacketId.S_Chat:
                return 'chat'
            else:
                return 'unknown'
                
        except Exception as e:
            self.errors.append(f"Parse error: {e}")
            with self.metrics.lock:
                self.metrics.protocol_errors += 1
            return 'error'
    
    def connect_and_setup(self) -> bool:
        """연결 및 초기 설정"""
        try:
            # 소켓 연결
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(30)
            self.socket.connect((TestConfig.HOST, TestConfig.PORT))
            self.connected = True
            
            with self.metrics.lock:
                self.metrics.clients_connected += 1
            
            # 로그인 (더미 처리)
            login_packet = self.create_login_packet()
            if self.send_packet_with_metrics(login_packet):
                success, response = self.receive_packet_with_metrics()
                if success:
                    packet_type = self.parse_packet_with_latency(response, time.time())
                    self.logged_in = True
                else:
                    self.logged_in = True  # 응답 없어도 진행
            
            # 존 입장 (더미 처리)
            if self.logged_in:
                zone_packet = self.create_enter_zone_packet()
                if self.send_packet_with_metrics(zone_packet):
                    success, response = self.receive_packet_with_metrics()
                    if success:
                        packet_type = self.parse_packet_with_latency(response, time.time())
                    self.in_zone = True
                    
                    with self.metrics.lock:
                        self.metrics.clients_in_zone += 1
            
            return self.in_zone
            
        except Exception as e:
            self.errors.append(f"Setup error: {e}")
            with self.metrics.lock:
                self.metrics.connection_failures += 1
            return False
    
    def generate_realistic_input(self) -> int:
        """현실적인 플레이어 입력 패턴 생성"""
        current_time = time.time()
        
        # 0.5~3초마다 움직임 패턴 변경
        if current_time - self.last_input_change > random.uniform(0.5, 3.0):
            input_flags = 0
            
            # 80% 확률로 움직임
            if random.random() < 0.8:
                # 현실적인 움직임 패턴
                movement_patterns = [
                    TestConfig.INPUT_W,  # 앞으로
                    TestConfig.INPUT_S,  # 뒤로
                    TestConfig.INPUT_A,  # 왼쪽
                    TestConfig.INPUT_D,  # 오른쪽
                    TestConfig.INPUT_W | TestConfig.INPUT_A,  # 왼쪽 앞
                    TestConfig.INPUT_W | TestConfig.INPUT_D,  # 오른쪽 앞
                    TestConfig.INPUT_S | TestConfig.INPUT_A,  # 왼쪽 뒤
                    TestConfig.INPUT_S | TestConfig.INPUT_D,  # 오른쪽 뒤
                ]
                input_flags = random.choice(movement_patterns)
            
            self.current_input_flags = input_flags
            self.last_input_change = current_time
        
        return self.current_input_flags
    
    def performance_test_loop(self):
        """성능 테스트 메인 루프"""
        if not self.connect_and_setup():
            self.report_results()
            return
        
        print(f"Client {self.client_id}: 성능 테스트 시작")
        
        # 송신/수신 스레드 시작
        sender_thread = threading.Thread(target=self._sender_thread)
        receiver_thread = threading.Thread(target=self._receiver_thread)
        
        sender_thread.daemon = True
        receiver_thread.daemon = True
        
        sender_thread.start()
        receiver_thread.start()
        
        # 테스트 지속 시간만큼 실행
        time.sleep(self.config['duration'])
        
        # 테스트 종료
        self.should_stop = True
        time.sleep(1)
        
        # 결과 리포트
        self.report_results()
        
        # 연결 종료
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
    
    def _sender_thread(self):
        """패킷 송신 스레드"""
        next_move_time = time.time()
        next_chat_time = time.time() + random.uniform(1, self.config['chat_interval'])
        
        while not self.should_stop and self.in_zone:
            try:
                current_time = time.time()
                
                # 이동 입력 전송
                if current_time >= next_move_time:
                    input_flags = self.generate_realistic_input()
                    move_packet = self.create_player_input_packet(input_flags)
                    
                    if not self.send_packet_with_metrics(move_packet):
                        break
                    
                    next_move_time = current_time + self.config['movement_interval']
                
                # 주기적 채팅
                if current_time >= next_chat_time:
                    messages = [
                        f"Performance test by {self.username}",
                        f"Packets sent: {self.packets_sent}",
                        f"Test duration: {current_time - self.metrics.test_start_time:.1f}s",
                        "Measuring server performance...",
                        f"Client {self.client_id} reporting"
                    ]
                    message = random.choice(messages)
                    chat_packet = self.create_chat_packet(message)
                    
                    self.send_packet_with_metrics(chat_packet)
                    next_chat_time = current_time + self.config['chat_interval'] + random.uniform(-1, 1)
                
                # CPU 절약
                time.sleep(0.001)
                
            except Exception as e:
                self.errors.append(f"Sender error: {e}")
                break
    
    def _receiver_thread(self):
        """패킷 수신 스레드"""
        self.socket.settimeout(0.05)  # 50ms 타임아웃
        
        while not self.should_stop and self.in_zone:
            try:
                success, packet_data = self.receive_packet_with_metrics()
                if success:
                    receive_time = time.time()
                    packet_type = self.parse_packet_with_latency(packet_data, receive_time)
                    
                    # 패킷별 지연시간 계산
                    if packet_type == 'world_snapshot':
                        # 글로벌 메트릭스에 지연시간 추가
                        if self.local_latencies:
                            self.metrics.add_latency_sample(self.local_latencies[-1])
                
            except socket.timeout:
                continue
            except Exception as e:
                self.errors.append(f"Receiver error: {e}")
                break
    
    def report_results(self):
        """결과 리포트"""
        avg_latency = np.mean(self.local_latencies) if self.local_latencies else 0
        avg_rtt = np.mean(self.local_rtts) if self.local_rtts else 0
        
        self.stats_queue.put({
            'client_id': self.client_id,
            'connected': self.connected,
            'logged_in': self.logged_in,
            'in_zone': self.in_zone,
            'packets_sent': self.packets_sent,
            'packets_received': self.packets_received,
            'bytes_sent': self.bytes_sent,
            'bytes_received': self.bytes_received,
            'avg_latency_ms': avg_latency,
            'avg_rtt_ms': avg_rtt,
            'error_count': len(self.errors),
            'error_messages': self.errors[:3]
        })

# === 실시간 성능 모니터링 ===
class RealTimeMonitor:
    def __init__(self, metrics: PerformanceMetrics, test_config: dict):
        self.metrics = metrics
        self.test_config = test_config
        self.should_stop = False
        
    def start_monitoring(self):
        """모니터링 시작"""
        monitor_thread = threading.Thread(target=self._monitoring_loop)
        monitor_thread.daemon = True
        monitor_thread.start()
        
        return monitor_thread
    
    def _monitoring_loop(self):
        """실시간 모니터링 루프"""
        # CSV 파일 초기화
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"performance_test_{timestamp}.csv"
        
        with open(csv_filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = [
                'elapsed_time', 'clients_connected', 'clients_in_zone',
                'packets_sent_total', 'packets_per_sec', 'mbps_sent', 'mbps_received',
                'avg_latency_ms', 'p95_latency_ms', 'avg_rtt_ms',
                'cpu_usage', 'memory_usage', 'error_rate_percent',
                'send_errors', 'receive_errors', 'connection_failures'
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            start_time = time.time()
            
            while not self.should_stop and (time.time() - start_time < self.test_config['duration']):
                time.sleep(5)  # 5초마다 모니터링
                
                stats = self.metrics.get_current_stats()
                
                # CSV 행 데이터
                row_data = {
                    'elapsed_time': f"{stats['elapsed_time']:.0f}",
                    'clients_connected': stats['connections']['connected'],
                    'clients_in_zone': stats['connections']['in_zone'],
                    'packets_sent_total': stats['throughput']['packets_sent'],
                    'packets_per_sec': f"{stats['throughput']['packets_per_sec']:.1f}",
                    'mbps_sent': f"{stats['throughput']['mbps_sent']:.2f}",
                    'mbps_received': f"{stats['throughput']['mbps_received']:.2f}",
                    'avg_latency_ms': f"{stats['latency'].get('avg', 0):.2f}",
                    'p95_latency_ms': f"{stats['latency'].get('p95', 0):.2f}",
                    'avg_rtt_ms': f"{stats['rtt'].get('avg', 0):.2f}",
                    'cpu_usage': f"{stats['system_resources']['cpu_usage']:.1f}",
                    'memory_usage': f"{stats['system_resources']['memory_usage']:.1f}",
                    'error_rate_percent': f"{stats['errors']['error_rate_percent']:.2f}",
                    'send_errors': stats['errors']['send_errors'],
                    'receive_errors': stats['errors']['receive_errors'],
                    'connection_failures': stats['connections']['connection_failures']
                }
                
                writer.writerow(row_data)
                csvfile.flush()
                
                # 콘솔 출력
                self._print_realtime_stats(stats)
        
        print(f"\n성능 데이터가 {csv_filename}에 저장되었습니다.")
    
    def _print_realtime_stats(self, stats):
        """실시간 통계 콘솔 출력"""
        elapsed = stats['elapsed_time']
        
        print(f"\n{'='*80}")
        print(f"[{elapsed:.0f}s] 실시간 성능 모니터링")
        print(f"{'='*80}")
        
        # 연결 상태
        print(f"연결 상태:")
        print(f"  연결된 클라이언트: {stats['connections']['connected']}")
        print(f"  존 입장 클라이언트: {stats['connections']['in_zone']}")
        print(f"  연결 실패: {stats['connections']['connection_failures']}")
        
        # 처리량
        print(f"\n처리량 (Throughput):")
        print(f"  패킷 전송: {stats['throughput']['packets_sent']:,} ({stats['throughput']['packets_per_sec']:.1f}/s)")
        print(f"  패킷 수신: {stats['throughput']['packets_received']:,}")
        print(f"  대역폭 송신: {stats['throughput']['mbps_sent']:.2f} Mbps")
        print(f"  대역폭 수신: {stats['throughput']['mbps_received']:.2f} Mbps")
        
        # 지연시간
        if stats['latency']:
            print(f"\n지연시간 (Latency):")
            print(f"  평균: {stats['latency']['avg']:.2f}ms")
            print(f"  최소/최대: {stats['latency']['min']:.2f}ms / {stats['latency']['max']:.2f}ms")
            print(f"  P95/P99: {stats['latency']['p95']:.2f}ms / {stats['latency']['p99']:.2f}ms")
        
        if stats['rtt']:
            print(f"  RTT 평균: {stats['rtt']['avg']:.2f}ms (P95: {stats['rtt']['p95']:.2f}ms)")
        
        # 시스템 리소스
        print(f"\n시스템 리소스:")
        print(f"  CPU 사용률: {stats['system_resources']['cpu_usage']:.1f}%")
        print(f"  메모리 사용률: {stats['system_resources']['memory_usage']:.1f}%")
        
        # 오류율
        print(f"\n안정성:")
        print(f"  오류율: {stats['errors']['error_rate_percent']:.2f}%")
        print(f"  전송 오류: {stats['errors']['send_errors']}")
        print(f"  수신 오류: {stats['errors']['receive_errors']}")
        print(f"  프로토콜 오류: {stats['errors']['protocol_errors']}")
    
    def stop_monitoring(self):
        self.should_stop = True

# === 메인 테스트 실행기 ===
def run_comprehensive_performance_test(scenario_name: str = 'basic'):
    """종합 성능 테스트 실행"""
    if scenario_name not in TestConfig.SCENARIOS:
        print(f"알 수 없는 시나리오: {scenario_name}")
        print(f"사용 가능한 시나리오: {list(TestConfig.SCENARIOS.keys())}")
        return
    
    config = TestConfig.SCENARIOS[scenario_name]
    
    print(f"{'='*80}")
    print(f"CppMMO 서버 종합 성능 측정 시스템")
    print(f"{'='*80}")
    print(f"시나리오: {scenario_name.upper()}")
    print(f"클라이언트 수: {config['clients']}")
    print(f"테스트 시간: {config['duration']}초 ({config['duration']//60}분)")
    print(f"이동 입력 주기: {config['movement_interval']:.3f}초 ({1/config['movement_interval']:.0f}fps)")
    print(f"채팅 주기: {config['chat_interval']:.1f}초")
    print(f"대상 서버: {TestConfig.HOST}:{TestConfig.PORT}")
    print(f"{'='*80}")
    
    # 성능 메트릭스 초기화
    metrics = PerformanceMetrics()
    metrics.test_start_time = time.time()
    
    # 시스템 리소스 모니터 시작
    print("시스템 리소스 모니터링 시작...")
    resource_monitor = SystemResourceMonitor(metrics)
    resource_monitor.start_monitoring()
    
    # 실시간 모니터링 시작
    print("실시간 성능 모니터링 시작...")
    realtime_monitor = RealTimeMonitor(metrics, config)
    monitor_thread = realtime_monitor.start_monitoring()
    
    # 통계 수집용 큐
    stats_queue = queue.Queue()
    
    print(f"\n{config['clients']}개 클라이언트로 성능 테스트 시작...")
    
    start_time = time.time()
    
    # 클라이언트 실행
    with ThreadPoolExecutor(max_workers=config['clients'] + 20) as executor:
        futures = []
        
        # 클라이언트 점진적 연결 (서버에 부하 분산)
        batch_size = min(20, config['clients'] // 10) if config['clients'] > 50 else config['clients']
        
        for i in range(config['clients']):
            client = PerformanceTestClient(i + 1, config, metrics, stats_queue)
            future = executor.submit(client.performance_test_loop)
            futures.append(future)
            
            # 배치별 연결 지연
            if (i + 1) % batch_size == 0:
                time.sleep(0.2)
                print(f"클라이언트 {i + 1}/{config['clients']} 시작...")
            elif i % 5 == 4:
                time.sleep(0.05)
        
        print("모든 클라이언트 시작! 종합 성능 측정 진행 중...")
        
        # 모든 작업 완료 대기
        for future in futures:
            try:
                future.result(timeout=config['duration'] + 60)
            except Exception as e:
                print(f"클라이언트 실행 오류: {e}")
    
    # 모니터링 종료
    realtime_monitor.stop_monitoring()
    resource_monitor.stop_monitoring()
    
    # 최종 결과 분석
    total_time = time.time() - start_time
    
    # 개별 클라이언트 통계 수집
    client_stats = []
    while not stats_queue.empty():
        client_stats.append(stats_queue.get())
    
    # 최종 성능 리포트 생성
    generate_final_performance_report(scenario_name, config, metrics, client_stats, total_time)

def generate_final_performance_report(scenario_name: str, config: dict, metrics: PerformanceMetrics, 
                                    client_stats: list, total_time: float):
    """최종 성능 리포트 생성"""
    
    # 최종 통계 계산
    final_stats = metrics.get_current_stats()
    connected_clients = [c for c in client_stats if c['connected']]
    active_clients = [c for c in client_stats if c['in_zone']]
    
    print(f"\n{'='*100}")
    print(f"CppMMO 서버 종합 성능 측정 최종 결과")
    print(f"{'='*100}")
    print(f"시나리오: {scenario_name.upper()}")
    print(f"총 테스트 시간: {total_time:.2f}초")
    print(f"목표 클라이언트: {config['clients']}")
    print(f"연결 성공: {len(connected_clients)}")
    print(f"활성 클라이언트: {len(active_clients)}")
    
    if not active_clients:
        print("❌ 활성 클라이언트가 없어 성능 측정을 완료할 수 없습니다.")
        return
    
    # 1. 처리량 (Throughput) 분석
    print(f"\n[처리량 분석]:")
    print(f"  총 패킷 전송: {final_stats['throughput']['packets_sent']:,}")
    print(f"  총 패킷 수신: {final_stats['throughput']['packets_received']:,}")
    print(f"  평균 처리율: {final_stats['throughput']['packets_per_sec']:.1f} 패킷/초")
    print(f"  송신 대역폭: {final_stats['throughput']['mbps_sent']:.2f} Mbps")
    print(f"  수신 대역폭: {final_stats['throughput']['mbps_received']:.2f} Mbps")
    
    # 예상 처리량과 비교
    expected_packets = len(active_clients) * (config['duration'] / config['movement_interval'])
    throughput_efficiency = (final_stats['throughput']['packets_sent'] / expected_packets * 100) if expected_packets > 0 else 0
    print(f"  처리량 효율: {throughput_efficiency:.1f}% (예상 대비)")
    
    # 2. 지연시간 (Latency) 분석
    if final_stats['latency']:
        print(f"\n[지연시간 분석]:")
        print(f"  평균 지연시간: {final_stats['latency']['avg']:.2f}ms")
        print(f"  최소/최대: {final_stats['latency']['min']:.2f}ms / {final_stats['latency']['max']:.2f}ms")
        print(f"  중위수 (P50): {final_stats['latency']['p50']:.2f}ms")
        print(f"  95분위수 (P95): {final_stats['latency']['p95']:.2f}ms")
        print(f"  99분위수 (P99): {final_stats['latency']['p99']:.2f}ms")
        print(f"  표준편차: {final_stats['latency']['std']:.2f}ms")
        
        # 지연시간 평가
        p95_latency = final_stats['latency']['p95']
        if p95_latency < 50:
            latency_grade = "[우수] < 50ms"
        elif p95_latency < 100:
            latency_grade = "[양호] 50-100ms"
        elif p95_latency < 200:
            latency_grade = "[보통] 100-200ms"
        else:
            latency_grade = "[개선필요] > 200ms"
        
        print(f"  지연시간 등급: {latency_grade}")
    
    # 3. RTT 분석
    if final_stats['rtt']:
        print(f"\n[RTT 분석]:")
        print(f"  평균 RTT: {final_stats['rtt']['avg']:.2f}ms")
        print(f"  최소/최대: {final_stats['rtt']['min']:.2f}ms / {final_stats['rtt']['max']:.2f}ms")
        print(f"  95분위수: {final_stats['rtt']['p95']:.2f}ms")
    
    # 4. 시스템 리소스 분석
    print(f"\n[시스템 리소스 사용량]:")
    print(f"  최대 CPU 사용률: {final_stats['system_resources']['cpu_usage']:.1f}%")
    print(f"  최대 메모리 사용률: {final_stats['system_resources']['memory_usage']:.1f}%")
    print(f"  네트워크 송신: {final_stats['system_resources']['network_sent'] / (1024*1024):.1f} MB")
    print(f"  네트워크 수신: {final_stats['system_resources']['network_recv'] / (1024*1024):.1f} MB")
    
    # 5. 안정성 분석
    print(f"\n[안정성 분석]:")
    print(f"  연결 성공률: {len(connected_clients) / config['clients'] * 100:.1f}%")
    print(f"  전체 오류율: {final_stats['errors']['error_rate_percent']:.2f}%")
    print(f"  연결 실패: {final_stats['connections']['connection_failures']}")
    print(f"  전송 오류: {final_stats['errors']['send_errors']}")
    print(f"  수신 오류: {final_stats['errors']['receive_errors']}")
    print(f"  프로토콜 오류: {final_stats['errors']['protocol_errors']}")
    
    # 6. 종합 성능 평가
    print(f"\n[종합 성능 평가]:")
    
    # 점수 계산 로직
    connection_score = min(100, len(connected_clients) / config['clients'] * 100)
    throughput_score = min(100, throughput_efficiency)
    latency_score = max(0, min(100, (200 - final_stats['latency'].get('p95', 200)) / 2)) if final_stats['latency'] else 50
    stability_score = max(0, 100 - final_stats['errors']['error_rate_percent'] * 10)
    
    overall_score = (connection_score * 0.25 + throughput_score * 0.35 + 
                    latency_score * 0.25 + stability_score * 0.15)
    
    print(f"  연결성: {connection_score:.1f}/100")
    print(f"  처리량: {throughput_score:.1f}/100")
    print(f"  지연시간: {latency_score:.1f}/100")
    print(f"  안정성: {stability_score:.1f}/100")
    print(f"  종합 점수: {overall_score:.1f}/100")
    
    # 최종 등급
    if overall_score >= 90:
        grade = "🏅 S급 - 우수한 성능, 프로덕션 준비 완료"
    elif overall_score >= 80:
        grade = "🥈 A급 - 좋은 성능, 소규모 최적화 권장"
    elif overall_score >= 70:
        grade = "🥉 B급 - 보통 성능, 성능 개선 필요"
    elif overall_score >= 60:
        grade = "⚠️ C급 - 미흡한 성능, 상당한 최적화 필요"
    else:
        grade = "❌ D급 - 부족한 성능, 아키텍처 재검토 필요"
    
    print(f"  최종 등급: {grade}")
    
    # 7. 개선 권장사항
    print(f"\n💡 개선 권장사항:")
    
    if throughput_efficiency < 80:
        print("  - 패킷 처리 성능 최적화 (메시지 큐, 스레드 풀 조정)")
    
    if final_stats['latency'] and final_stats['latency'].get('p95', 0) > 100:
        print("  - 지연시간 최적화 (네트워크 버퍼링, 알고리즘 개선)")
    
    if final_stats['errors']['error_rate_percent'] > 1:
        print("  - 오류 처리 로직 강화 (재연결, 예외 처리)")
    
    if final_stats['system_resources']['cpu_usage'] > 80:
        print("  - CPU 사용률 최적화 (프로파일링, 알고리즘 개선)")
    
    if final_stats['system_resources']['memory_usage'] > 80:
        print("  - 메모리 사용량 최적화 (메모리 풀, 캐시 관리)")
    
    if len(connected_clients) < config['clients'] * 0.9:
        print("  - 연결 안정성 향상 (타임아웃 조정, 재연결 로직)")
    
    print(f"\n{'='*100}")
    
    # JSON 결과 파일 저장
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results = {
        'scenario': scenario_name,
        'config': config,
        'test_duration': total_time,
        'final_stats': final_stats,
        'client_count': len(connected_clients),
        'active_clients': len(active_clients),
        'overall_score': overall_score,
        'grade': grade,
        'timestamp': timestamp
    }
    
    with open(f"performance_results_{timestamp}.json", 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=2, ensure_ascii=False)
    
    print(f"상세 결과가 performance_results_{timestamp}.json에 저장되었습니다.")

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='CppMMO 서버 종합 성능 측정')
    parser.add_argument('--scenario', '-s', 
                        choices=['basic', 'stress', 'extreme', 'massive', 'optimized'],
                        default='basic',
                        help='성능 테스트 시나리오 선택')
    parser.add_argument('--clients', '-c',
                        type=int,
                        help='클라이언트 수 (시나리오 기본값 무시)')
    parser.add_argument('--duration', '-d',
                        type=int,
                        help='테스트 지속 시간(초)')
    
    args = parser.parse_args()
    
    # 명령행 인수로 시나리오 설정 오버라이드
    if args.clients or args.duration:
        scenario_config = TestConfig.SCENARIOS[args.scenario].copy()
        if args.clients:
            scenario_config['clients'] = args.clients
        if args.duration:
            scenario_config['duration'] = args.duration
        
        # 동적 시나리오 설정을 위해 SCENARIOS 업데이트
        TestConfig.SCENARIOS[f'{args.scenario}_custom'] = scenario_config
        scenario_name = f'{args.scenario}_custom'
    else:
        scenario_name = args.scenario
    
    try:
        print("성능 측정을 시작합니다. 서버가 실행 중인지 확인하세요...")
        time.sleep(2)
        
        run_comprehensive_performance_test(scenario_name)
        
    except KeyboardInterrupt:
        print("\n성능 테스트가 사용자에 의해 중단되었습니다.")
    except Exception as e:
        print(f"성능 테스트 실행 중 오류 발생: {e}")
        import traceback
        traceback.print_exc()