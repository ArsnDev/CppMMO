#!/usr/bin/env python3
"""
실시간 이동 부하 테스트 - C_PlayerInput 패킷 기반
플레이어 로그인 → 존 입장 → 지속적인 이동 입력 전송
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
import json
from datetime import datetime
import csv

# 테스트 계정 로드
try:
    with open('load_test_accounts.json', 'r') as f:
        TEST_ACCOUNTS = json.load(f)
    print(f"테스트 계정 {len(TEST_ACCOUNTS)}개 로드 완료")
except FileNotFoundError:
    print("load_test_accounts.json 파일이 없습니다. create_test_accounts.py를 먼저 실행하세요.")
    sys.exit(1)

# --- FlatBuffers 모듈 임포트 설정 ---
flatbuffers_module_base_path = os.path.abspath(os.path.dirname(__file__))
flatbuffers_module_path = os.path.join(flatbuffers_module_base_path, 'Protocol')

if flatbuffers_module_path not in sys.path:
    sys.path.append(flatbuffers_module_path)

try:
    import C_Login
    import C_EnterZone
    import C_PlayerInput
    import S_LoginSuccess
    import S_ZoneEntered
    import S_WorldSnapshot
    import Packet
    import PacketId
    import UnifiedPacket
    import Vec3
    import PlayerState
except ImportError as e:
    print(f"Error: Could not import FlatBuffers modules from {flatbuffers_module_path}. {e}")
    sys.exit(1)

# 테스트 설정
HOST = 'localhost'
PORT = 8080
NUM_CLIENTS = 1000  # 1000개 클라이언트로 극한 테스트
TEST_DURATION = 600  # 10분간 테스트
MOVEMENT_INTERVAL = 0.033  # ~30fps 입력 (33ms)
ZONE_ID = 1

# 입력 플래그 (WASD 비트마스크)
INPUT_W = 1
INPUT_S = 2  
INPUT_A = 4
INPUT_D = 8

# 글로벌 통계
global_stats = {
    'clients_connected': 0,
    'clients_in_zone': 0,
    'total_inputs_sent': 0,
    'total_snapshots_received': 0,
    'connection_failures': 0,
    'login_failures': 0,
    'zone_enter_failures': 0,
    'input_send_errors': 0,
    'snapshot_receive_errors': 0,
    'start_time': None,
    'errors': []
}
stats_lock = threading.Lock()

class MovementLoadTestClient:
    def __init__(self, client_id: int, stats_queue):
        self.client_id = client_id
        self.socket = None
        self.connected = False
        self.logged_in = False
        self.in_zone = False
        self.should_stop = False
        self.stats_queue = stats_queue
        
        # 플레이어 정보 (계정 풀에서 순환 선택)
        account_index = (client_id - 1) % len(TEST_ACCOUNTS)
        account = TEST_ACCOUNTS[account_index]
        self.player_id = account['player_id']
        self.session_ticket = account['session_ticket']
        self.username = account['username']
        self.sequence_number = 0
        
        # 통계
        self.connection_time = None
        self.login_time = None
        self.zone_enter_time = None
        self.inputs_sent = 0
        self.snapshots_received = 0
        self.errors = []
        
        # 현재 입력 상태
        self.current_input_flags = 0
        self.last_input_change = time.time()
        
    def create_login_packet(self):
        """C_Login 패킷 생성"""
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
        """C_EnterZone 패킷 생성"""
        builder = flatbuffers.Builder(0)
        
        C_EnterZone.C_EnterZoneStart(builder)
        C_EnterZone.C_EnterZoneAddZoneId(builder, ZONE_ID)
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
        """C_PlayerInput 패킷 생성"""
        builder = flatbuffers.Builder(0)
        
        self.sequence_number += 1
        current_time = int(time.time() * 1000)  # milliseconds
        
        # 마우스 위치 (임의)
        Vec3.Vec3Start(builder)
        Vec3.Vec3AddX(builder, random.uniform(-100, 100))
        Vec3.Vec3AddY(builder, random.uniform(-100, 100))
        Vec3.Vec3AddZ(builder, 0.0)
        mouse_pos_offset = Vec3.Vec3End(builder)
        
        C_PlayerInput.C_PlayerInputStart(builder)
        C_PlayerInput.C_PlayerInputAddTickNumber(builder, 0)  # 클라이언트는 0으로 전송
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
    
    def send_packet(self, packet_data: bytes) -> bool:
        """패킷 전송"""
        try:
            message_length = len(packet_data)
            length_prefix = struct.pack('<I', message_length)  # Little endian으로 변경
            full_message = length_prefix + packet_data
            self.socket.sendall(full_message)
            return True
        except Exception as e:
            self.errors.append(f"Send error: {e}")
            return False
    
    def receive_packet(self) -> tuple[bool, bytes]:
        """패킷 수신"""
        try:
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
            
            return True, message_data
        except Exception as e:
            self.errors.append(f"Receive error: {e}")
            return False, b''
    
    def parse_packet(self, packet_data: bytes):
        """패킷 파싱"""
        try:
            unified_packet = UnifiedPacket.UnifiedPacket.GetRootAsUnifiedPacket(packet_data, 0)
            packet_id = unified_packet.Id()
            
            if packet_id == PacketId.PacketId.S_LoginSuccess:
                return 'login_success', None
            elif packet_id == PacketId.PacketId.S_ZoneEntered:
                return 'zone_entered', None
            elif packet_id == PacketId.PacketId.S_WorldSnapshot:
                return 'world_snapshot', None
            else:
                return 'unknown', packet_id
                
        except Exception as e:
            self.errors.append(f"Parse error: {e}")
            return 'error', None
    
    def connect_and_setup(self) -> bool:
        """연결 및 초기 설정"""
        try:
            # 1. 소켓 연결
            start_time = time.time()
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(10)  # 10초 타임아웃
            self.socket.connect((HOST, PORT))
            self.connection_time = time.time() - start_time
            self.connected = True
            
            with stats_lock:
                global_stats['clients_connected'] += 1
            
            # 2. 로그인
            login_packet = self.create_login_packet()
            if not self.send_packet(login_packet):
                return False
            
            success, response = self.receive_packet()
            if not success:
                return False
            
            packet_type, _ = self.parse_packet(response)
            if packet_type != 'login_success':
                with stats_lock:
                    global_stats['login_failures'] += 1
                return False
            
            self.logged_in = True
            self.login_time = time.time() - start_time
            
            # 3. 존 입장
            zone_packet = self.create_enter_zone_packet()
            if not self.send_packet(zone_packet):
                return False
            
            success, response = self.receive_packet()
            if not success:
                return False
            
            packet_type, _ = self.parse_packet(response)
            if packet_type != 'zone_entered':
                with stats_lock:
                    global_stats['zone_enter_failures'] += 1
                return False
            
            self.in_zone = True
            self.zone_enter_time = time.time() - start_time
            
            with stats_lock:
                global_stats['clients_in_zone'] += 1
            
            return True
            
        except Exception as e:
            self.errors.append(f"Setup error: {e}")
            with stats_lock:
                global_stats['connection_failures'] += 1
            return False
    
    def generate_random_input(self) -> int:
        """랜덤 입력 생성 (WASD 조합)"""
        current_time = time.time()
        
        # 0.5~2초마다 입력 패턴 변경
        if current_time - self.last_input_change > random.uniform(0.5, 2.0):
            # 새로운 입력 패턴 생성
            input_flags = 0
            
            # 40% 확률로 움직임
            if random.random() < 0.4:
                # W/S 중 하나 선택
                if random.random() < 0.5:
                    input_flags |= INPUT_W
                else:
                    input_flags |= INPUT_S
                
                # A/D 중 하나 선택
                if random.random() < 0.5:
                    input_flags |= INPUT_A
                else:
                    input_flags |= INPUT_D
            
            self.current_input_flags = input_flags
            self.last_input_change = current_time
        
        return self.current_input_flags
    
    def movement_sender_thread(self):
        """지속적으로 이동 입력 전송"""
        while not self.should_stop and self.in_zone:
            try:
                input_flags = self.generate_random_input()
                packet = self.create_player_input_packet(input_flags)
                
                if self.send_packet(packet):
                    self.inputs_sent += 1
                    with stats_lock:
                        global_stats['total_inputs_sent'] += 1
                else:
                    with stats_lock:
                        global_stats['input_send_errors'] += 1
                    break
                
                time.sleep(MOVEMENT_INTERVAL)
                
            except Exception as e:
                self.errors.append(f"Movement sender error: {e}")
                break
    
    def snapshot_receiver_thread(self):
        """S_WorldSnapshot 수신"""
        self.socket.settimeout(0.1)  # 100ms 타임아웃
        
        while not self.should_stop and self.in_zone:
            try:
                success, packet_data = self.receive_packet()
                if not success:
                    continue
                
                packet_type, _ = self.parse_packet(packet_data)
                if packet_type == 'world_snapshot':
                    self.snapshots_received += 1
                    with stats_lock:
                        global_stats['total_snapshots_received'] += 1
                
            except socket.timeout:
                continue
            except Exception as e:
                self.errors.append(f"Snapshot receiver error: {e}")
                with stats_lock:
                    global_stats['snapshot_receive_errors'] += 1
                break
    
    def run_movement_test(self):
        """실시간 이동 테스트 실행"""
        if not self.connect_and_setup():
            self.report_results()
            return
        
        print(f"Client {self.client_id}: 준비 완료, 이동 테스트 시작")
        
        # 송신/수신 스레드 시작
        sender_thread = threading.Thread(target=self.movement_sender_thread)
        receiver_thread = threading.Thread(target=self.snapshot_receiver_thread)
        
        sender_thread.daemon = True
        receiver_thread.daemon = True
        
        sender_thread.start()
        receiver_thread.start()
        
        # TEST_DURATION 동안 실행
        time.sleep(TEST_DURATION)
        
        # 테스트 종료
        self.should_stop = True
        
        # 스레드 종료 대기
        time.sleep(1)
        
        # 결과 리포트
        self.report_results()
        
        # 연결 종료
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
    
    def report_results(self):
        """결과 리포트"""
        self.stats_queue.put({
            'client_id': self.client_id,
            'connected': self.connected,
            'logged_in': self.logged_in,
            'in_zone': self.in_zone,
            'connection_time': self.connection_time,
            'login_time': self.login_time,
            'zone_enter_time': self.zone_enter_time,
            'inputs_sent': self.inputs_sent,
            'snapshots_received': self.snapshots_received,
            'errors': len(self.errors),
            'error_messages': self.errors[:5]  # 처음 5개 에러만 저장
        })

def print_realtime_stats():
    """실시간 통계 출력 및 CSV 로깅"""
    start_time = time.time()
    
    # CSV 파일 초기화
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"load_test_stats_{timestamp}.csv"
    
    with open(csv_filename, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = ['elapsed_time', 'clients_connected', 'clients_in_zone', 
                     'total_inputs_sent', 'inputs_per_sec', 'total_snapshots_received', 
                     'snapshots_per_sec', 'input_errors', 'snapshot_errors']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        
        while time.time() - start_time < TEST_DURATION:
            time.sleep(5)  # 5초마다 출력
            
            with stats_lock:
                elapsed = time.time() - global_stats['start_time']
                inputs_per_sec = global_stats['total_inputs_sent'] / elapsed if elapsed > 0 else 0
                snapshots_per_sec = global_stats['total_snapshots_received'] / elapsed if elapsed > 0 else 0
                
                stats_row = {
                    'elapsed_time': f"{elapsed:.0f}",
                    'clients_connected': global_stats['clients_connected'],
                    'clients_in_zone': global_stats['clients_in_zone'],
                    'total_inputs_sent': global_stats['total_inputs_sent'],
                    'inputs_per_sec': f"{inputs_per_sec:.1f}",
                    'total_snapshots_received': global_stats['total_snapshots_received'],
                    'snapshots_per_sec': f"{snapshots_per_sec:.1f}",
                    'input_errors': global_stats['input_send_errors'],
                    'snapshot_errors': global_stats['snapshot_receive_errors']
                }
                
                writer.writerow(stats_row)
                csvfile.flush()  # 즉시 파일에 쓰기
                
                print(f"\n[{elapsed:.0f}s] 실시간 통계:")
                print(f"  연결된 클라이언트: {global_stats['clients_connected']}")
                print(f"  존 입장 클라이언트: {global_stats['clients_in_zone']}")
                print(f"  총 입력 전송: {global_stats['total_inputs_sent']} ({inputs_per_sec:.1f}/s)")
                print(f"  총 스냅샷 수신: {global_stats['total_snapshots_received']} ({snapshots_per_sec:.1f}/s)")
                print(f"  입력 전송 오류: {global_stats['input_send_errors']}")
                print(f"  스냅샷 수신 오류: {global_stats['snapshot_receive_errors']}") 
                
    print(f"\n실시간 통계가 {csv_filename}에 저장되었습니다.")

def run_movement_load_test():
    print(f"=== 실시간 이동 부하 테스트 ===")
    print(f"클라이언트 수: {NUM_CLIENTS}")
    print(f"테스트 시간: {TEST_DURATION}초")
    print(f"이동 입력 간격: {MOVEMENT_INTERVAL:.3f}초 (~{1/MOVEMENT_INTERVAL:.0f}fps)")
    print(f"예상 총 입력량: {NUM_CLIENTS * (TEST_DURATION / MOVEMENT_INTERVAL):.0f}개")
    print(f"대상 서버: {HOST}:{PORT}")
    
    # 글로벌 통계 초기화
    global_stats['start_time'] = time.time()
    
    # 통계 수집용 큐
    stats_queue = queue.Queue()
    
    # 실시간 통계 출력 스레드
    stats_thread = threading.Thread(target=print_realtime_stats)
    stats_thread.daemon = True
    stats_thread.start()
    
    start_time = time.time()
    
    # 부하 테스트 실행
    with ThreadPoolExecutor(max_workers=NUM_CLIENTS) as executor:
        print(f"\n{NUM_CLIENTS}개 클라이언트 생성 및 테스트 시작...")
        
        futures = []
        for i in range(NUM_CLIENTS):
            client = MovementLoadTestClient(i + 1, stats_queue)
            future = executor.submit(client.run_movement_test)
            futures.append(future)
            
            # 연결 부하 분산 (50개씩 0.5초 간격)
            if i % 50 == 49:
                time.sleep(0.5)
                print(f"클라이언트 {i + 1}/{NUM_CLIENTS} 시작...")
            elif i % 10 == 9:
                time.sleep(0.1)
        
        print("모든 클라이언트 시작! 실시간 이동 부하 테스트 진행 중...")
        
        # 모든 작업 완료 대기
        for future in futures:
            try:
                future.result(timeout=TEST_DURATION + 30)
            except Exception as e:
                print(f"클라이언트 실행 오류: {e}")
    
    # 최종 결과 분석
    total_time = time.time() - start_time
    
    # 통계 수집
    stats = []
    while not stats_queue.empty():
        stats.append(stats_queue.get())
    
    # 결과 분석
    connected_clients = [s for s in stats if s['connected']]
    zone_entered_clients = [s for s in stats if s['in_zone']]
    
    print(f"\n{'='*80}")
    print(f"실시간 이동 부하 테스트 최종 결과")
    print(f"{'='*80}")
    print(f"총 테스트 시간: {total_time:.2f}초")
    print(f"연결 성공: {len(connected_clients)}/{NUM_CLIENTS}")
    print(f"존 입장 성공: {len(zone_entered_clients)}/{NUM_CLIENTS}")
    
    if connected_clients:
        connection_times = [s['connection_time'] for s in connected_clients if s['connection_time']]
        if connection_times:
            print(f"평균 연결 시간: {statistics.mean(connection_times):.3f}초")
    
    if zone_entered_clients:
        total_inputs = sum(s['inputs_sent'] for s in zone_entered_clients)
        total_snapshots = sum(s['snapshots_received'] for s in zone_entered_clients)
        
        print(f"\n성능 통계:")
        print(f"총 입력 전송: {total_inputs:,}")
        print(f"총 스냅샷 수신: {total_snapshots:,}")
        print(f"초당 입력 처리율: {total_inputs / TEST_DURATION:.2f} inputs/sec")
        print(f"초당 스냅샷 처리율: {total_snapshots / TEST_DURATION:.2f} snapshots/sec")
        
        avg_inputs = total_inputs / len(zone_entered_clients)
        avg_snapshots = total_snapshots / len(zone_entered_clients)
        print(f"클라이언트당 평균 입력: {avg_inputs:.1f}개")
        print(f"클라이언트당 평균 스냅샷: {avg_snapshots:.1f}개")
        
        # 대역폭 계산 (대략적)
        avg_input_packet_size = 200  # bytes
        avg_snapshot_packet_size = 500  # bytes
        input_bandwidth_mbps = (total_inputs * avg_input_packet_size * 8) / (TEST_DURATION * 1024 * 1024)
        snapshot_bandwidth_mbps = (total_snapshots * avg_snapshot_packet_size * 8) / (TEST_DURATION * 1024 * 1024)
        
        print(f"\n대역폭 사용량 (추정):")
        print(f"업로드 (입력): {input_bandwidth_mbps:.2f} Mbps")
        print(f"다운로드 (스냅샷): {snapshot_bandwidth_mbps:.2f} Mbps")
        print(f"총 대역폭: {input_bandwidth_mbps + snapshot_bandwidth_mbps:.2f} Mbps")
        
        # 성능 평가
        expected_inputs = NUM_CLIENTS * (TEST_DURATION / MOVEMENT_INTERVAL)
        input_success_rate = (total_inputs / expected_inputs) * 100 if expected_inputs > 0 else 0
        
        print(f"\n성능 평가:")
        print(f"입력 전송 성공률: {input_success_rate:.1f}%")
        
        if input_success_rate >= 95 and len(zone_entered_clients) >= NUM_CLIENTS * 0.9:
            print("*** 우수 - 고부하 상황에서도 안정적 처리")
        elif input_success_rate >= 80 and len(zone_entered_clients) >= NUM_CLIENTS * 0.7:
            print("** 양호 - 대부분의 트래픽 처리 성공")
        else:
            print("* 개선 필요 - 성능 병목 현상 발생")
    
    print(f"{'='*80}")

if __name__ == "__main__":
    try:
        run_movement_load_test()
    except KeyboardInterrupt:
        print("\n테스트가 사용자에 의해 중단되었습니다.")
    except Exception as e:
        print(f"테스트 실행 중 오류 발생: {e}")