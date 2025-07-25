#!/usr/bin/env python3
"""
인증 우회 부하테스트 - move/chat 패킷만으로 부하테스트
더미 데이터로 간단하게 접속하여 실시간 부하테스트 수행
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
from datetime import datetime

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

# 테스트 설정
HOST = 'localhost'
PORT = 8080
NUM_CLIENTS = 50  # 적당한 수로 시작
TEST_DURATION = 120  # 2분간 테스트
MOVEMENT_INTERVAL = 0.05  # 20fps 입력
CHAT_INTERVAL = 5.0  # 5초마다 채팅
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
    'total_chats_sent': 0,
    'total_snapshots_received': 0,
    'connection_failures': 0,
    'login_failures': 0,
    'zone_enter_failures': 0,
    'packet_send_errors': 0,
    'packet_receive_errors': 0,
    'start_time': None,
    'errors': []
}
stats_lock = threading.Lock()

class NoAuthStressClient:
    def __init__(self, client_id: int, stats_queue):
        self.client_id = client_id
        self.socket = None
        self.connected = False
        self.logged_in = False
        self.in_zone = False
        self.should_stop = False
        self.stats_queue = stats_queue
        
        # 더미 플레이어 정보 (인증 우회)
        self.player_id = 1000 + client_id  # 간단한 ID
        self.session_ticket = f"dummy_session_{client_id}"
        self.username = f"TestUser{client_id:03d}"
        self.sequence_number = 0
        
        # 통계
        self.connection_time = None
        self.login_time = None
        self.zone_enter_time = None
        self.inputs_sent = 0
        self.chats_sent = 0
        self.snapshots_received = 0
        self.errors = []
        
        # 현재 입력 상태
        self.current_input_flags = 0
        self.last_input_change = time.time()
        self.last_chat_time = time.time()
        
    def create_login_packet(self):
        """C_Login 패킷 생성 (더미 데이터)"""
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
        current_time = int(time.time() * 1000)
        
        # 마우스 위치 (랜덤)
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
        """C_Chat 패킷 생성"""
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
    
    def send_packet(self, packet_data: bytes) -> bool:
        """패킷 전송"""
        try:
            message_length = len(packet_data)
            length_prefix = struct.pack('<I', message_length)
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
            elif packet_id == PacketId.PacketId.S_Chat:
                return 'chat', None
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
            self.socket.settimeout(30)  # 30초 타임아웃
            self.socket.connect((HOST, PORT))
            self.connection_time = time.time() - start_time
            self.connected = True
            
            with stats_lock:
                global_stats['clients_connected'] += 1
            
            # 2. 로그인 (더미)
            login_packet = self.create_login_packet()
            if not self.send_packet(login_packet):
                return False
            
            success, response = self.receive_packet()
            if success:
                packet_type, _ = self.parse_packet(response)
                if packet_type == 'login_success':
                    self.logged_in = True
                    self.login_time = time.time() - start_time
                elif packet_type == 'error':
                    # 로그인 실패해도 계속 진행 (인증 우회)
                    self.logged_in = True
                    self.login_time = time.time() - start_time
                    print(f"Client {self.client_id}: 로그인 응답 오류, 하지만 계속 진행")
            else:
                # 응답 없어도 계속 진행
                self.logged_in = True
                self.login_time = time.time() - start_time
                print(f"Client {self.client_id}: 로그인 응답 없음, 하지만 계속 진행")
            
            # 3. 존 입장
            zone_packet = self.create_enter_zone_packet()
            if not self.send_packet(zone_packet):
                return False
            
            success, response = self.receive_packet()
            if success:
                packet_type, _ = self.parse_packet(response)
                if packet_type == 'zone_entered':
                    self.in_zone = True
                    self.zone_enter_time = time.time() - start_time
                elif packet_type == 'error':
                    # 존 입장 실패해도 계속 진행
                    self.in_zone = True
                    self.zone_enter_time = time.time() - start_time
                    print(f"Client {self.client_id}: 존 입장 응답 오류, 하지만 계속 진행")
            else:
                # 응답 없어도 계속 진행
                self.in_zone = True
                self.zone_enter_time = time.time() - start_time
                print(f"Client {self.client_id}: 존 입장 응답 없음, 하지만 계속 진행")
            
            with stats_lock:
                global_stats['clients_in_zone'] += 1
            
            return True
            
        except Exception as e:
            self.errors.append(f"Setup error: {e}")
            with stats_lock:
                global_stats['connection_failures'] += 1
            return False
    
    def generate_random_input(self) -> int:
        """랜덤 입력 생성"""
        current_time = time.time()
        
        # 1~3초마다 입력 패턴 변경
        if current_time - self.last_input_change > random.uniform(1.0, 3.0):
            input_flags = 0
            
            # 60% 확률로 움직임
            if random.random() < 0.6:
                # 방향 랜덤 선택
                directions = [INPUT_W, INPUT_S, INPUT_A, INPUT_D]
                num_directions = random.randint(1, 2)  # 1~2개 방향 조합
                selected = random.sample(directions, num_directions)
                for direction in selected:
                    input_flags |= direction
            
            self.current_input_flags = input_flags
            self.last_input_change = current_time
        
        return self.current_input_flags
    
    def packet_sender_thread(self):
        """패킷 전송 스레드 (move + chat)"""
        while not self.should_stop and self.in_zone:
            try:
                current_time = time.time()
                packets_sent = 0
                
                # 1. 이동 입력 전송
                input_flags = self.generate_random_input()
                move_packet = self.create_player_input_packet(input_flags)
                if self.send_packet(move_packet):
                    self.inputs_sent += 1
                    packets_sent += 1
                    with stats_lock:
                        global_stats['total_inputs_sent'] += 1
                else:
                    with stats_lock:
                        global_stats['packet_send_errors'] += 1
                    break
                
                # 2. 주기적 채팅 전송
                if current_time - self.last_chat_time >= CHAT_INTERVAL:
                    chat_messages = [
                        f"Hello from {self.username}!",
                        f"Test message #{self.chats_sent + 1}",
                        f"Client {self.client_id} reporting",
                        "How's everyone doing?",
                        "This is a stress test!",
                        f"Time: {current_time:.1f}"
                    ]
                    message = random.choice(chat_messages)
                    chat_packet = self.create_chat_packet(message)
                    
                    if self.send_packet(chat_packet):
                        self.chats_sent += 1
                        packets_sent += 1
                        with stats_lock:
                            global_stats['total_chats_sent'] += 1
                        self.last_chat_time = current_time
                    else:
                        with stats_lock:
                            global_stats['packet_send_errors'] += 1
                
                # 전송 간격 조절
                time.sleep(MOVEMENT_INTERVAL)
                
            except Exception as e:
                self.errors.append(f"Sender error: {e}")
                break
    
    def packet_receiver_thread(self):
        """패킷 수신 스레드"""
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
                # 다른 패킷들도 카운트할 수 있음
                
            except socket.timeout:
                continue
            except Exception as e:
                self.errors.append(f"Receiver error: {e}")
                with stats_lock:
                    global_stats['packet_receive_errors'] += 1
                break
    
    def run_stress_test(self):
        """스트레스 테스트 실행"""
        if not self.connect_and_setup():
            self.report_results()
            return
        
        print(f"Client {self.client_id}: 준비 완료, 스트레스 테스트 시작")
        
        # 송신/수신 스레드 시작
        sender_thread = threading.Thread(target=self.packet_sender_thread)
        receiver_thread = threading.Thread(target=self.packet_receiver_thread)
        
        sender_thread.daemon = True
        receiver_thread.daemon = True
        
        sender_thread.start()
        receiver_thread.start()
        
        # TEST_DURATION 동안 실행
        time.sleep(TEST_DURATION)
        
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
            'chats_sent': self.chats_sent,
            'snapshots_received': self.snapshots_received,
            'errors': len(self.errors),
            'error_messages': self.errors[:3]
        })

def print_realtime_stats():
    """실시간 통계 출력"""
    start_time = time.time()
    
    # CSV 파일 초기화
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"no_auth_stress_stats_{timestamp}.csv"
    
    with open(csv_filename, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = ['elapsed_time', 'clients_connected', 'clients_in_zone', 
                     'total_inputs_sent', 'inputs_per_sec', 'total_chats_sent', 'chats_per_sec',
                     'total_snapshots_received', 'snapshots_per_sec', 'send_errors', 'receive_errors']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        
        while time.time() - start_time < TEST_DURATION:
            time.sleep(5)  # 5초마다 출력
            
            with stats_lock:
                elapsed = time.time() - global_stats['start_time']
                inputs_per_sec = global_stats['total_inputs_sent'] / elapsed if elapsed > 0 else 0
                chats_per_sec = global_stats['total_chats_sent'] / elapsed if elapsed > 0 else 0
                snapshots_per_sec = global_stats['total_snapshots_received'] / elapsed if elapsed > 0 else 0
                
                stats_row = {
                    'elapsed_time': f"{elapsed:.0f}",
                    'clients_connected': global_stats['clients_connected'],
                    'clients_in_zone': global_stats['clients_in_zone'],
                    'total_inputs_sent': global_stats['total_inputs_sent'],
                    'inputs_per_sec': f"{inputs_per_sec:.1f}",
                    'total_chats_sent': global_stats['total_chats_sent'],
                    'chats_per_sec': f"{chats_per_sec:.1f}",
                    'total_snapshots_received': global_stats['total_snapshots_received'],
                    'snapshots_per_sec': f"{snapshots_per_sec:.1f}",
                    'send_errors': global_stats['packet_send_errors'],
                    'receive_errors': global_stats['packet_receive_errors']
                }
                
                writer.writerow(stats_row)
                csvfile.flush()
                
                print(f"\n[{elapsed:.0f}s] 실시간 통계:")
                print(f"  연결된 클라이언트: {global_stats['clients_connected']}")
                print(f"  존 입장 클라이언트: {global_stats['clients_in_zone']}")
                print(f"  총 이동 입력: {global_stats['total_inputs_sent']} ({inputs_per_sec:.1f}/s)")
                print(f"  총 채팅 전송: {global_stats['total_chats_sent']} ({chats_per_sec:.1f}/s)")
                print(f"  총 스냅샷 수신: {global_stats['total_snapshots_received']} ({snapshots_per_sec:.1f}/s)")
                print(f"  전송 오류: {global_stats['packet_send_errors']}")
                print(f"  수신 오류: {global_stats['packet_receive_errors']}")
                
    print(f"\n실시간 통계가 {csv_filename}에 저장되었습니다.")

def run_no_auth_stress_test():
    print(f"=== 인증 우회 스트레스 테스트 ===")
    print(f"클라이언트 수: {NUM_CLIENTS}")
    print(f"테스트 시간: {TEST_DURATION}초")
    print(f"이동 입력 간격: {MOVEMENT_INTERVAL:.3f}초")
    print(f"채팅 전송 간격: {CHAT_INTERVAL:.1f}초")
    print(f"대상 서버: {HOST}:{PORT}")
    print("주의: 이 테스트는 인증을 우회하여 더미 데이터로 접속합니다.")
    
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
            client = NoAuthStressClient(i + 1, stats_queue)
            future = executor.submit(client.run_stress_test)
            futures.append(future)
            
            # 연결 부하 분산 (10개씩 0.1초 간격)
            if i % 10 == 9:
                time.sleep(0.1)
                print(f"클라이언트 {i + 1}/{NUM_CLIENTS} 시작...")
        
        print("모든 클라이언트 시작! 인증 우회 스트레스 테스트 진행 중...")
        
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
    print(f"인증 우회 스트레스 테스트 최종 결과")
    print(f"{'='*80}")
    print(f"총 테스트 시간: {total_time:.2f}초")
    print(f"연결 성공: {len(connected_clients)}/{NUM_CLIENTS}")
    print(f"존 입장 성공: {len(zone_entered_clients)}/{NUM_CLIENTS}")
    
    if zone_entered_clients:
        total_inputs = sum(s['inputs_sent'] for s in zone_entered_clients)
        total_chats = sum(s['chats_sent'] for s in zone_entered_clients)
        total_snapshots = sum(s['snapshots_received'] for s in zone_entered_clients)
        
        print(f"\n성능 통계:")
        print(f"총 이동 입력: {total_inputs:,}")
        print(f"총 채팅 전송: {total_chats:,}")
        print(f"총 스냅샷 수신: {total_snapshots:,}")
        print(f"초당 이동 처리율: {total_inputs / TEST_DURATION:.2f} inputs/sec")
        print(f"초당 채팅 처리율: {total_chats / TEST_DURATION:.2f} chats/sec")
        print(f"초당 스냅샷 처리율: {total_snapshots / TEST_DURATION:.2f} snapshots/sec")
        
        # 성능 평가
        expected_inputs = NUM_CLIENTS * (TEST_DURATION / MOVEMENT_INTERVAL)
        input_success_rate = (total_inputs / expected_inputs) * 100 if expected_inputs > 0 else 0
        
        print(f"\n성능 평가:")
        print(f"입력 전송 성공률: {input_success_rate:.1f}%")
        print(f"클라이언트당 평균 이동: {total_inputs / len(zone_entered_clients):.1f}개")
        print(f"클라이언트당 평균 채팅: {total_chats / len(zone_entered_clients):.1f}개")
        
        if input_success_rate >= 90 and len(zone_entered_clients) >= NUM_CLIENTS * 0.8:
            print("*** 우수 - 서버가 부하를 안정적으로 처리")
        elif input_success_rate >= 70 and len(zone_entered_clients) >= NUM_CLIENTS * 0.6:
            print("** 양호 - 대부분의 트래픽 처리 성공")
        else:
            print("* 개선 필요 - 성능 병목 현상 발생")
    
    print(f"{'='*80}")

if __name__ == "__main__":
    try:
        run_no_auth_stress_test()
    except KeyboardInterrupt:
        print("\n테스트가 사용자에 의해 중단되었습니다.")
    except Exception as e:
        print(f"테스트 실행 중 오류 발생: {e}")