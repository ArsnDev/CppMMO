#!/usr/bin/env python3
"""
시스템 리소스 전용 모니터링 도구
서버 성능 테스트 중 시스템 리소스를 실시간으로 모니터링하고 기록
"""
import psutil
import time
import json
import csv
import threading
from datetime import datetime
from collections import deque
import matplotlib.pyplot as plt
import numpy as np

class SystemResourceMonitor:
    def __init__(self, monitor_duration=300):
        self.monitor_duration = monitor_duration
        self.should_stop = False
        
        # 데이터 저장
        self.timestamps = deque()
        self.cpu_usage = deque()
        self.memory_usage = deque()
        self.memory_available = deque()
        self.network_sent = deque()
        self.network_recv = deque()
        self.disk_read = deque()
        self.disk_write = deque()
        
        # 서버 프로세스별 모니터링
        self.server_processes = {}
        self.server_cpu = deque()
        self.server_memory = deque()
        
        # 초기값
        self.initial_network = psutil.net_io_counters()
        self.initial_disk = psutil.disk_io_counters()
        
    def find_server_processes(self):
        """CppMMO 관련 프로세스 찾기"""
        processes = {}
        
        for proc in psutil.process_iter(['pid', 'name', 'cmdline', 'cpu_percent', 'memory_percent']):
            try:
                name = proc.info.get('name', '').lower() if proc.info.get('name') else ''
                cmdline_list = proc.info.get('cmdline', []) if proc.info.get('cmdline') else []
                cmdline = ' '.join(str(cmd) for cmd in cmdline_list).lower()
                
                # CppMMO 서버 프로세스 식별
                if any(keyword in name or keyword in cmdline for keyword in 
                       ['cppmmo', 'gameserver', 'authserver', 'server.exe']):
                    processes[proc.info['pid']] = {
                        'process': psutil.Process(proc.info['pid']),
                        'name': proc.info.get('name', 'Unknown'),
                        'cmdline': cmdline[:100]  # 100자로 제한
                    }
                    
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                continue
                
        return processes
    
    def get_system_stats(self):
        """현재 시스템 통계 수집"""
        current_time = time.time()
        
        # CPU 사용률
        cpu_percent = psutil.cpu_percent(interval=0.1)
        
        # 메모리 사용률
        memory = psutil.virtual_memory()
        memory_percent = memory.percent
        memory_available_gb = memory.available / (1024**3)
        
        # 네트워크 통계
        current_network = psutil.net_io_counters()
        net_sent_mb = (current_network.bytes_sent - self.initial_network.bytes_sent) / (1024**2)
        net_recv_mb = (current_network.bytes_recv - self.initial_network.bytes_recv) / (1024**2)
        
        # 디스크 I/O
        current_disk = psutil.disk_io_counters()
        if current_disk and self.initial_disk:
            disk_read_mb = (current_disk.read_bytes - self.initial_disk.read_bytes) / (1024**2)
            disk_write_mb = (current_disk.write_bytes - self.initial_disk.write_bytes) / (1024**2)
        else:
            disk_read_mb = disk_write_mb = 0
        
        # 서버 프로세스 통계
        server_cpu_total = 0
        server_memory_total = 0
        
        for pid, proc_info in self.server_processes.items():
            try:
                proc = proc_info['process']
                if proc.is_running():
                    server_cpu_total += proc.cpu_percent()
                    server_memory_total += proc.memory_percent()
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        return {
            'timestamp': current_time,
            'cpu_usage': cpu_percent,
            'memory_usage': memory_percent,
            'memory_available_gb': memory_available_gb,
            'network_sent_mb': net_sent_mb,
            'network_recv_mb': net_recv_mb,
            'disk_read_mb': disk_read_mb,
            'disk_write_mb': disk_write_mb,
            'server_cpu': server_cpu_total,
            'server_memory': server_memory_total
        }
    
    def start_monitoring(self, output_prefix="system_monitor"):
        """모니터링 시작"""
        print("시스템 리소스 모니터링 시작...")
        
        # 서버 프로세스 찾기
        self.server_processes = self.find_server_processes()
        if self.server_processes:
            print(f"모니터링 대상 프로세스 {len(self.server_processes)}개 발견:")
            for pid, info in self.server_processes.items():
                print(f"  PID {pid}: {info['name']} - {info['cmdline']}")
        else:
            print("CppMMO 서버 프로세스를 찾을 수 없습니다. 전체 시스템만 모니터링합니다.")
        
        # CSV 파일 준비
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"{output_prefix}_{timestamp}.csv"
        
        with open(csv_filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = [
                'timestamp', 'elapsed_time', 
                'cpu_usage', 'memory_usage', 'memory_available_gb',
                'network_sent_mb', 'network_recv_mb', 
                'disk_read_mb', 'disk_write_mb',
                'server_cpu', 'server_memory'
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            start_time = time.time()
            next_display_time = start_time + 10  # 10초마다 콘솔 출력
            
            while not self.should_stop and (time.time() - start_time < self.monitor_duration):
                stats = self.get_system_stats()
                elapsed = time.time() - start_time
                
                # 데이터 저장
                self.timestamps.append(stats['timestamp'])
                self.cpu_usage.append(stats['cpu_usage'])
                self.memory_usage.append(stats['memory_usage'])
                self.memory_available.append(stats['memory_available_gb'])
                self.network_sent.append(stats['network_sent_mb'])
                self.network_recv.append(stats['network_recv_mb'])
                self.disk_read.append(stats['disk_read_mb'])
                self.disk_write.append(stats['disk_write_mb'])
                self.server_cpu.append(stats['server_cpu'])
                self.server_memory.append(stats['server_memory'])
                
                # CSV 기록
                row = {
                    'timestamp': stats['timestamp'],
                    'elapsed_time': f"{elapsed:.1f}",
                    'cpu_usage': f"{stats['cpu_usage']:.1f}",
                    'memory_usage': f"{stats['memory_usage']:.1f}",
                    'memory_available_gb': f"{stats['memory_available_gb']:.2f}",
                    'network_sent_mb': f"{stats['network_sent_mb']:.2f}",
                    'network_recv_mb': f"{stats['network_recv_mb']:.2f}",
                    'disk_read_mb': f"{stats['disk_read_mb']:.2f}",
                    'disk_write_mb': f"{stats['disk_write_mb']:.2f}",
                    'server_cpu': f"{stats['server_cpu']:.1f}",
                    'server_memory': f"{stats['server_memory']:.1f}"
                }
                writer.writerow(row)
                csvfile.flush()
                
                # 주기적 콘솔 출력
                if time.time() >= next_display_time:
                    self.print_current_stats(stats, elapsed)
                    next_display_time = time.time() + 10
                
                time.sleep(2)  # 2초 간격
        
        print(f"\n시스템 모니터링 완료. 데이터가 {csv_filename}에 저장되었습니다.")
        
        # 그래프 생성
        self.generate_graphs(output_prefix, timestamp)
        
        return csv_filename
    
    def print_current_stats(self, stats, elapsed):
        """현재 통계 콘솔 출력"""
        print(f"\n[{elapsed:.0f}s] 시스템 리소스 현황:")
        print(f"  CPU: {stats['cpu_usage']:.1f}%")
        print(f"  메모리: {stats['memory_usage']:.1f}% (가용: {stats['memory_available_gb']:.1f}GB)")
        print(f"  네트워크: ↑{stats['network_sent_mb']:.1f}MB / ↓{stats['network_recv_mb']:.1f}MB")
        print(f"  디스크: 읽기 {stats['disk_read_mb']:.1f}MB / 쓰기 {stats['disk_write_mb']:.1f}MB")
        
        if stats['server_cpu'] > 0 or stats['server_memory'] > 0:
            print(f"  서버 프로세스: CPU {stats['server_cpu']:.1f}% / 메모리 {stats['server_memory']:.1f}%")
    
    def generate_graphs(self, output_prefix, timestamp):
        """성능 그래프 생성"""
        if len(self.timestamps) < 5:
            print("데이터가 부족하여 그래프를 생성할 수 없습니다.")
            return
        
        try:
            # 시간 축 계산 (경과 시간)
            start_time = self.timestamps[0]
            elapsed_times = [(t - start_time) / 60 for t in self.timestamps]  # 분 단위
            
            # 2x2 서브플롯 생성
            fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
            fig.suptitle(f'시스템 리소스 모니터링 결과 - {timestamp}', fontsize=16)
            
            # 1. CPU 사용률
            ax1.plot(elapsed_times, self.cpu_usage, 'b-', label='전체 CPU', linewidth=2)
            if any(cpu > 0 for cpu in self.server_cpu):
                ax1.plot(elapsed_times, self.server_cpu, 'r-', label='서버 CPU', linewidth=2)
            ax1.set_title('CPU 사용률')
            ax1.set_xlabel('시간 (분)')
            ax1.set_ylabel('사용률 (%)')
            ax1.grid(True, alpha=0.3)
            ax1.legend()
            ax1.set_ylim(0, 100)
            
            # 2. 메모리 사용률
            ax2.plot(elapsed_times, self.memory_usage, 'g-', label='전체 메모리', linewidth=2)
            if any(mem > 0 for mem in self.server_memory):
                ax2.plot(elapsed_times, self.server_memory, 'orange', label='서버 메모리', linewidth=2)
            ax2.set_title('메모리 사용률')
            ax2.set_xlabel('시간 (분)')
            ax2.set_ylabel('사용률 (%)')
            ax2.grid(True, alpha=0.3)
            ax2.legend()
            ax2.set_ylim(0, 100)
            
            # 3. 네트워크 사용량
            ax3.plot(elapsed_times, self.network_sent, 'purple', label='송신', linewidth=2)
            ax3.plot(elapsed_times, self.network_recv, 'cyan', label='수신', linewidth=2)
            ax3.set_title('네트워크 사용량')
            ax3.set_xlabel('시간 (분)')
            ax3.set_ylabel('데이터량 (MB)')
            ax3.grid(True, alpha=0.3)
            ax3.legend()
            
            # 4. 디스크 I/O
            ax4.plot(elapsed_times, self.disk_read, 'brown', label='읽기', linewidth=2)
            ax4.plot(elapsed_times, self.disk_write, 'pink', label='쓰기', linewidth=2)
            ax4.set_title('디스크 I/O')
            ax4.set_xlabel('시간 (분)')
            ax4.set_ylabel('데이터량 (MB)')
            ax4.grid(True, alpha=0.3)
            ax4.legend()
            
            plt.tight_layout()
            
            # 그래프 저장
            graph_filename = f"{output_prefix}_graphs_{timestamp}.png"
            plt.savefig(graph_filename, dpi=300, bbox_inches='tight')
            print(f"성능 그래프가 {graph_filename}에 저장되었습니다.")
            
            # 통계 요약 생성
            self.generate_summary_report(output_prefix, timestamp)
            
        except Exception as e:
            print(f"그래프 생성 중 오류: {e}")
    
    def generate_summary_report(self, output_prefix, timestamp):
        """요약 리포트 생성"""
        if not self.cpu_usage:
            return
        
        summary = {
            'monitoring_duration_minutes': len(self.timestamps) * 2 / 60,  # 2초 간격
            'cpu_stats': {
                'avg': float(np.mean(self.cpu_usage)),
                'max': float(np.max(self.cpu_usage)),
                'min': float(np.min(self.cpu_usage)),
                'std': float(np.std(self.cpu_usage))
            },
            'memory_stats': {
                'avg': float(np.mean(self.memory_usage)),
                'max': float(np.max(self.memory_usage)),
                'min': float(np.min(self.memory_usage)),
                'std': float(np.std(self.memory_usage))
            },
            'network_stats': {
                'total_sent_mb': float(self.network_sent[-1]) if self.network_sent else 0,
                'total_recv_mb': float(self.network_recv[-1]) if self.network_recv else 0,
                'avg_sent_rate_mbps': float(np.mean(np.diff(self.network_sent))) * 30 if len(self.network_sent) > 1 else 0,  # 2초 간격 -> 30배
                'avg_recv_rate_mbps': float(np.mean(np.diff(self.network_recv))) * 30 if len(self.network_recv) > 1 else 0
            },
            'disk_stats': {
                'total_read_mb': float(self.disk_read[-1]) if self.disk_read else 0,
                'total_write_mb': float(self.disk_write[-1]) if self.disk_write else 0
            }
        }
        
        # 서버 프로세스 통계 (데이터가 있는 경우)
        if any(cpu > 0 for cpu in self.server_cpu):
            summary['server_process_stats'] = {
                'avg_cpu': float(np.mean([cpu for cpu in self.server_cpu if cpu > 0])),
                'max_cpu': float(np.max(self.server_cpu)),
                'avg_memory': float(np.mean([mem for mem in self.server_memory if mem > 0])),
                'max_memory': float(np.max(self.server_memory))
            }
        
        # JSON 파일로 저장
        summary_filename = f"{output_prefix}_summary_{timestamp}.json"
        with open(summary_filename, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2, ensure_ascii=False)
        
        print(f"요약 리포트가 {summary_filename}에 저장되었습니다.")
        
        # 콘솔에 요약 출력
        self.print_summary_report(summary)
    
    def print_summary_report(self, summary):
        """요약 리포트 콘솔 출력"""
        print(f"\n{'='*60}")
        print(f"시스템 리소스 모니터링 요약")
        print(f"{'='*60}")
        print(f"모니터링 시간: {summary['monitoring_duration_minutes']:.1f}분")
        
        print(f"\n💻 CPU 사용률:")
        cpu = summary['cpu_stats']
        print(f"  평균: {cpu['avg']:.1f}% | 최대: {cpu['max']:.1f}% | 최소: {cpu['min']:.1f}%")
        
        print(f"\n🧠 메모리 사용률:")
        mem = summary['memory_stats']
        print(f"  평균: {mem['avg']:.1f}% | 최대: {mem['max']:.1f}% | 최소: {mem['min']:.1f}%")
        
        print(f"\n🌐 네트워크 사용량:")
        net = summary['network_stats']
        print(f"  총 송신: {net['total_sent_mb']:.1f}MB | 총 수신: {net['total_recv_mb']:.1f}MB")
        print(f"  평균 송신 속도: {net['avg_sent_rate_mbps']:.2f}MB/s")
        print(f"  평균 수신 속도: {net['avg_recv_rate_mbps']:.2f}MB/s")
        
        print(f"\n💾 디스크 I/O:")
        disk = summary['disk_stats']
        print(f"  총 읽기: {disk['total_read_mb']:.1f}MB | 총 쓰기: {disk['total_write_mb']:.1f}MB")
        
        if 'server_process_stats' in summary:
            print(f"\n🎮 서버 프로세스:")
            server = summary['server_process_stats']
            print(f"  평균 CPU: {server['avg_cpu']:.1f}% | 최대 CPU: {server['max_cpu']:.1f}%")
            print(f"  평균 메모리: {server['avg_memory']:.1f}% | 최대 메모리: {server['max_memory']:.1f}%")
        
        print(f"{'='*60}")
    
    def stop_monitoring(self):
        """모니터링 중지"""
        self.should_stop = True

def run_system_monitor(duration=300):
    """시스템 모니터 실행"""
    monitor = SystemResourceMonitor(duration)
    
    try:
        csv_file = monitor.start_monitoring()
        return csv_file
    except KeyboardInterrupt:
        print("\n모니터링이 사용자에 의해 중단되었습니다.")
        monitor.stop_monitoring()
    except Exception as e:
        print(f"모니터링 중 오류 발생: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='시스템 리소스 모니터링')
    parser.add_argument('--duration', '-d', type=int, default=300,
                        help='모니터링 시간 (초, 기본값: 300)')
    
    args = parser.parse_args()
    
    print(f"시스템 리소스를 {args.duration}초 동안 모니터링합니다...")
    print("Ctrl+C로 언제든 중단할 수 있습니다.")
    
    run_system_monitor(args.duration)