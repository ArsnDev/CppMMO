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

# --- FlatBuffers 모듈 임포트 설정 ---
flatbuffers_module_base_path = os.path.abspath(os.path.dirname(__file__))
flatbuffers_module_path = os.path.join(flatbuffers_module_base_path, 'Protocol')

if flatbuffers_module_path not in sys.path:
    sys.path.append(flatbuffers_module_path)

try:
    import C_Chat
    import S_Chat
    import Packet
    import PacketId
    import UnifiedPacket
except ImportError as e:
    print(f"Error: Could not import FlatBuffers modules from {flatbuffers_module_path}. {e}")
    sys.exit(1)

HOST = 'localhost'
PORT = 8080
NUM_CLIENTS = 1000
TEST_DURATION = 120  # 2분 테스트
MESSAGE_INTERVAL = 0.5  # 0.5초마다 메시지 전송 (매우 공격적)

# 글로벌 통계 수집
global_stats = {
    'messages_sent_total': 0,
    'messages_received_total': 0,
    'connections_active': 0,
    'connections_failed': 0,
    'start_time': None,
    'errors': []
}
stats_lock = threading.Lock()

class ExtremeStressClient:
    def __init__(self, client_id: int, stats_queue):
        self.client_id = client_id
        self.socket = None
        self.connected = False
        self.messages_sent = 0
        self.messages_received = 0
        self.should_stop = False
        self.stats_queue = stats_queue
        self.connection_time = None
        self.last_send_time = time.time()
        self.send_errors = 0
        self.receive_errors = 0
        
    def create_chat_message(self, message_text: str):
        builder = flatbuffers.Builder(0)
        message_offset = builder.CreateString(message_text)
        
        C_Chat.C_ChatStart(builder)
        C_Chat.C_ChatAddMessage(builder, message_offset)
        C_Chat.C_ChatAddCommandId(builder, random.randint(1, 1000000))
        c_chat_packet_offset = C_Chat.C_ChatEnd(builder)
        
        UnifiedPacket.UnifiedPacketStart(builder)
        UnifiedPacket.UnifiedPacketAddId(builder, PacketId.PacketId.C_Chat)
        UnifiedPacket.UnifiedPacketAddDataType(builder, Packet.Packet.C_Chat)
        UnifiedPacket.UnifiedPacketAddData(builder, c_chat_packet_offset)
        unified_packet_offset = UnifiedPacket.UnifiedPacketEnd(builder)
        
        builder.Finish(unified_packet_offset)
        return builder.Output()
    
    def parse_received_message(self, buffer: bytes):
        try:
            unified_packet = UnifiedPacket.UnifiedPacket.GetRootAsUnifiedPacket(buffer, 0)
            packet_id = unified_packet.Id()
            data_type = unified_packet.DataType()
            
            if packet_id == PacketId.PacketId.S_Chat and data_type == Packet.Packet.S_Chat:
                s_chat_packet = S_Chat.S_Chat()
                s_chat_packet.Init(unified_packet.Data().Bytes, unified_packet.Data().Pos)
                message = s_chat_packet.Message().decode('utf-8')
                return True, message
            return False, None
        except Exception as e:
            return False, None
    
    def connect(self):
        try:
            start_time = time.time()
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5)  # 5초 연결 타임아웃
            self.socket.connect((HOST, PORT))
            self.connection_time = time.time() - start_time
            self.connected = True
            
            with stats_lock:
                global_stats['connections_active'] += 1
            
            return True
        except Exception as e:
            with stats_lock:
                global_stats['connections_failed'] += 1
                global_stats['errors'].append(f"Client {self.client_id} connection failed: {e}")
            return False
    
    def aggressive_message_sender(self):
        """매우 공격적으로 메시지 전송 (0.5초마다)"""
        while not self.should_stop and self.connected:
            try:
                current_time = time.time()
                message = f"Client{self.client_id}_Msg{self.messages_sent + 1}_{int(current_time)}"
                
                flatbuffers_message = self.create_chat_message(message)
                message_length = len(flatbuffers_message)
                length_prefix = struct.pack('>I', message_length)
                full_message = length_prefix + flatbuffers_message
                
                self.socket.sendall(full_message)
                self.messages_sent += 1
                self.last_send_time = current_time
                
                with stats_lock:
                    global_stats['messages_sent_total'] += 1
                
                # 짧은 간격으로 메시지 전송
                time.sleep(MESSAGE_INTERVAL)
                
            except Exception as e:
                self.send_errors += 1
                with stats_lock:
                    global_stats['errors'].append(f"Client {self.client_id} send error: {e}")
                break
    
    def aggressive_message_receiver(self):
        """메시지 수신"""
        self.socket.settimeout(0.1)  # 매우 짧은 타임아웃
        
        while not self.should_stop and self.connected:
            try:
                length_data = self.socket.recv(4)
                if not length_data or len(length_data) < 4:
                    continue
                    
                message_length = struct.unpack('>I', length_data)[0]
                message_data = self.socket.recv(message_length)
                
                if not message_data:
                    continue
                
                success, message = self.parse_received_message(message_data)
                
                if success:
                    self.messages_received += 1
                    with stats_lock:
                        global_stats['messages_received_total'] += 1
                    
            except socket.timeout:
                continue
            except Exception as e:
                self.receive_errors += 1
                break
    
    def run_extreme_test(self):
        """극한 테스트 실행"""
        if not self.connect():
            self.stats_queue.put({
                'client_id': self.client_id,
                'connected': False,
                'connection_time': None,
                'messages_sent': 0,
                'messages_received': 0,
                'send_errors': 0,
                'receive_errors': 0
            })
            return
        
        # 수신 스레드 시작
        receive_thread = threading.Thread(target=self.aggressive_message_receiver)
        receive_thread.daemon = True
        receive_thread.start()
        
        # 송신 스레드 시작
        send_thread = threading.Thread(target=self.aggressive_message_sender)
        send_thread.daemon = True
        send_thread.start()
        
        # TEST_DURATION 동안 실행
        time.sleep(TEST_DURATION)
        
        # 테스트 종료
        self.should_stop = True
        
        # 잠시 대기 후 통계 수집
        time.sleep(1)
        
        # 결과 리포트
        self.stats_queue.put({
            'client_id': self.client_id,
            'connected': True,
            'connection_time': self.connection_time,
            'messages_sent': self.messages_sent,
            'messages_received': self.messages_received,
            'send_errors': self.send_errors,
            'receive_errors': self.receive_errors
        })
        
        # 연결 종료
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.connected = False
            
        with stats_lock:
            global_stats['connections_active'] -= 1

def print_realtime_stats():
    """실시간 통계 출력"""
    start_time = time.time()
    
    while time.time() - start_time < TEST_DURATION:
        time.sleep(5)  # 5초마다 출력
        
        with stats_lock:
            elapsed = time.time() - global_stats['start_time']
            msgs_per_sec = global_stats['messages_sent_total'] / elapsed if elapsed > 0 else 0
            recv_per_sec = global_stats['messages_received_total'] / elapsed if elapsed > 0 else 0
            
            print(f"\n[{elapsed:.0f}s] 실시간 통계:")
            print(f"  활성 연결: {global_stats['connections_active']}")
            print(f"  전송: {global_stats['messages_sent_total']} ({msgs_per_sec:.1f}/s)")
            print(f"  수신: {global_stats['messages_received_total']} ({recv_per_sec:.1f}/s)")
            print(f"  연결 실패: {global_stats['connections_failed']}")
            print(f"  오류 수: {len(global_stats['errors'])}")

def run_extreme_stress_test():
    print(f"=== 극한 스트레스 테스트: {NUM_CLIENTS}개 클라이언트 ===")
    print(f"테스트 시간: {TEST_DURATION}초")
    print(f"메시지 간격: {MESSAGE_INTERVAL}초")
    print(f"예상 메시지량: {NUM_CLIENTS * (TEST_DURATION / MESSAGE_INTERVAL):.0f}개")
    print(f"대상 서버: {HOST}:{PORT}")
    
    # 글로벌 통계 초기화
    global_stats['start_time'] = time.time()
    global_stats['messages_sent_total'] = 0
    global_stats['messages_received_total'] = 0
    global_stats['connections_active'] = 0
    global_stats['connections_failed'] = 0
    global_stats['errors'] = []
    
    # 통계 수집용 큐
    stats_queue = queue.Queue()
    
    # 실시간 통계 출력 스레드
    stats_thread = threading.Thread(target=print_realtime_stats)
    stats_thread.daemon = True
    stats_thread.start()
    
    start_time = time.time()
    
    # ThreadPoolExecutor로 1000개 클라이언트 동시 실행
    with ThreadPoolExecutor(max_workers=NUM_CLIENTS) as executor:
        print(f"\n{NUM_CLIENTS}개 클라이언트 생성 및 연결 중...")
        
        # 모든 클라이언트 생성 및 테스트 시작
        futures = []
        for i in range(NUM_CLIENTS):
            client = ExtremeStressClient(i + 1, stats_queue)
            future = executor.submit(client.run_extreme_test)
            futures.append(future)
            
            # 연결 부하 분산을 위한 아주 작은 지연
            if i % 50 == 49:
                time.sleep(0.01)
        
        print(f"모든 클라이언트 시작! 극한 테스트 진행 중...")
        print("실시간 모니터링 시작...")
        
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
    failed_connections = [s for s in stats if not s['connected']]
    
    if connected_clients:
        connection_times = [s['connection_time'] for s in connected_clients if s['connection_time']]
        total_sent = sum(s['messages_sent'] for s in connected_clients)
        total_received = sum(s['messages_received'] for s in connected_clients)
        total_send_errors = sum(s['send_errors'] for s in connected_clients)
        total_receive_errors = sum(s['receive_errors'] for s in connected_clients)
    else:
        connection_times = []
        total_sent = 0
        total_received = 0
        total_send_errors = 0
        total_receive_errors = 0
    
    print(f"\n{'='*80}")
    print(f"극한 스트레스 테스트 최종 결과")
    print(f"{'='*80}")
    print(f"총 테스트 시간: {total_time:.2f}초")
    print(f"성공적으로 연결된 클라이언트: {len(connected_clients)}/{NUM_CLIENTS}")
    print(f"연결 실패한 클라이언트: {len(failed_connections)}")
    
    if connection_times:
        print(f"평균 연결 시간: {statistics.mean(connection_times):.3f}초")
        print(f"최대 연결 시간: {max(connection_times):.3f}초")
    
    print(f"\n메시지 처리 통계:")
    print(f"총 전송된 메시지: {total_sent:,}")
    print(f"총 수신된 메시지: {total_received:,}")
    print(f"전송 오류: {total_send_errors:,}")
    print(f"수신 오류: {total_receive_errors:,}")
    
    if total_sent > 0:
        delivery_rate = (total_received / total_sent) * 100
        print(f"메시지 전달률: {delivery_rate:.2f}%")
    
    if TEST_DURATION > 0:
        messages_per_second_sent = total_sent / TEST_DURATION
        messages_per_second_received = total_received / TEST_DURATION
        print(f"초당 전송 처리율: {messages_per_second_sent:.2f} msg/sec")
        print(f"초당 수신 처리율: {messages_per_second_received:.2f} msg/sec")
    
    if connected_clients:
        avg_sent = total_sent / len(connected_clients)
        avg_received = total_received / len(connected_clients)
        print(f"클라이언트당 평균 전송: {avg_sent:.1f}개")
        print(f"클라이언트당 평균 수신: {avg_received:.1f}개")
    
    print(f"\n성능 등급:")
    if len(connected_clients) >= NUM_CLIENTS * 0.95 and delivery_rate >= 90:
        print("*** 우수 - 고부하 상황에서도 안정적 처리")
    elif len(connected_clients) >= NUM_CLIENTS * 0.8 and delivery_rate >= 70:
        print("** 양호 - 대부분의 트래픽 처리 성공")
    else:
        print("* 개선 필요 - 일부 트래픽 손실 발생")
    
    # 오류 요약
    if global_stats['errors']:
        print(f"\n오류 요약 (최근 10개):")
        for error in global_stats['errors'][-10:]:
            print(f"  - {error}")
    
    print(f"{'='*80}")

if __name__ == "__main__":
    print("WARNING: 이 테스트는 서버에 극한의 부하를 가합니다!")
    print("서버 리소스를 모니터링하면서 진행하세요.")
    
    confirm = input("계속 진행하시겠습니까? (y/N): ")
    if confirm.lower() != 'y':
        print("테스트가 취소되었습니다.")
        sys.exit(0)
    
    run_extreme_stress_test()