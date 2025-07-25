#!/usr/bin/env python3
"""
CppMMO 서버 확장성(Scalability) 테스트 실행 스크립트
다양한 인원수로 성능 테스트를 실행하여 확장성을 분석합니다.
"""
import subprocess
import sys
import time
import json
import os
from datetime import datetime
import argparse
import psutil
from pathlib import Path

class ScalabilityTestRunner:
    def __init__(self):
        self.test_results = {}
        self.test_start_time = None
        self.server_process = None
        
    def check_server_status(self, host='localhost', port=8080):
        """서버 상태 확인"""
        import socket
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            result = sock.connect_ex((host, port))
            sock.close()
            return result == 0
        except Exception:
            return False
    
    def wait_for_server_stabilization(self, wait_time=30):
        """서버 안정화 대기"""
        print(f"서버 안정화를 위해 {wait_time}초 대기 중...")
        
        for remaining in range(wait_time, 0, -1):
            print(f"\r남은 시간: {remaining}초", end="", flush=True)
            time.sleep(1)
        
        print("\n대기 완료")
    
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
    
    def monitor_server_resources(self):
        """서버 리소스 모니터링"""
        if not self.server_process:
            self.server_process = self.find_server_process()
        
        if self.server_process and self.server_process.is_running():
            try:
                cpu_percent = self.server_process.cpu_percent()
                memory_info = self.server_process.memory_info()
                memory_percent = self.server_process.memory_percent()
                
                return {
                    'cpu_percent': cpu_percent,
                    'memory_mb': memory_info.rss / 1024 / 1024,
                    'memory_percent': memory_percent,
                    'status': 'running'
                }
            except psutil.NoSuchProcess:
                self.server_process = None
                return {'status': 'not_found'}
        
        return {'status': 'not_running'}
    
    def run_performance_test(self, scenario, client_count, custom_duration=None):
        """성능 테스트 실행"""
        print(f"\n{'='*80}")
        print(f"성능 테스트 실행: {scenario.upper()} 시나리오, {client_count}명 클라이언트")
        print(f"{'='*80}")
        
        # 기존 comprehensive_performance_test.py의 설정을 동적으로 수정
        test_config = self.create_dynamic_test_config(scenario, client_count, custom_duration)
        
        # 임시 설정 파일 생성
        temp_config_file = f"temp_test_config_{scenario}_{client_count}.json"
        with open(temp_config_file, 'w', encoding='utf-8') as f:
            json.dump(test_config, f, indent=2)
        
        try:
            # 서버 리소스 확인
            server_status = self.monitor_server_resources()
            print(f"서버 상태: {server_status.get('status', 'unknown')}")
            if server_status.get('status') == 'running':
                print(f"테스트 시작 전 서버 리소스: CPU {server_status.get('cpu_percent', 0):.1f}%, "
                      f"메모리 {server_status.get('memory_mb', 0):.1f}MB")
            
            # 성능 테스트 실행 (comprehensive_performance_test.py 수정 버전)
            cmd = [
                sys.executable, 
                'comprehensive_performance_test.py', 
                '--scenario', scenario,
                '--clients', str(client_count)
            ]
            
            if custom_duration:
                cmd.extend(['--duration', str(custom_duration)])
            
            print(f"실행 명령어: {' '.join(cmd)}")
            
            start_time = time.time()
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=test_config['duration'] + 300  # 5분 여유시간
            )
            end_time = time.time()
            
            test_duration = end_time - start_time
            
            if result.returncode == 0:
                print(f"✅ 테스트 완료 (소요시간: {test_duration:.1f}초)")
                
                # 결과 파일 찾기
                result_files = self.find_latest_result_files()
                
                return {
                    'success': True,
                    'scenario': scenario,
                    'client_count': client_count,
                    'test_duration': test_duration,
                    'stdout': result.stdout[-1000:],  # 마지막 1000자만 저장
                    'stderr': result.stderr[-500:] if result.stderr else "",
                    'result_files': result_files,
                    'server_status_before': server_status
                }
            else:
                print(f"❌ 테스트 실패 (반환코드: {result.returncode})")
                print(f"오류 메시지: {result.stderr}")
                
                return {
                    'success': False,
                    'scenario': scenario,
                    'client_count': client_count,
                    'test_duration': test_duration,
                    'error': result.stderr,
                    'stdout': result.stdout[-1000:],
                    'server_status_before': server_status
                }
                
        except subprocess.TimeoutExpired:
            print(f"⏰ 테스트 타임아웃 (제한시간: {test_config['duration'] + 300}초)")
            return {
                'success': False,
                'scenario': scenario,
                'client_count': client_count,
                'error': 'Test timeout',
                'timeout': True
            }
        except Exception as e:
            print(f"💥 테스트 실행 중 예외: {e}")
            return {
                'success': False,
                'scenario': scenario,
                'client_count': client_count,
                'error': str(e),
                'exception': True
            }
        finally:
            # 임시 파일 정리
            if os.path.exists(temp_config_file):
                os.remove(temp_config_file)
    
    def create_dynamic_test_config(self, scenario, client_count, custom_duration=None):
        """동적 테스트 설정 생성"""
        # 클라이언트 수에 따른 동적 조정
        if client_count <= 50:
            base_duration = 120  # 2분
            movement_interval = 0.05  # 20fps
            chat_interval = 10.0
        elif client_count <= 200:
            base_duration = 180  # 3분
            movement_interval = 0.05  # 20fps
            chat_interval = 8.0
        elif client_count <= 500:
            base_duration = 240  # 4분
            movement_interval = 0.067  # 15fps (부하 감소)
            chat_interval = 12.0
        else:
            base_duration = 300  # 5분
            movement_interval = 0.1  # 10fps (부하 감소)
            chat_interval = 15.0
        
        # 시나리오별 조정
        if scenario == 'stress':
            movement_interval *= 0.66  # 빠른 입력
            chat_interval *= 0.8  # 더 빈번한 채팅
        elif scenario == 'extreme':
            movement_interval *= 0.5  # 매우 빠른 입력
            chat_interval *= 0.6  # 매우 빈번한 채팅
        
        duration = custom_duration if custom_duration else base_duration
        
        return {
            'scenario': scenario,
            'clients': client_count,
            'duration': duration,
            'movement_interval': movement_interval,
            'chat_interval': chat_interval
        }
    
    def find_latest_result_files(self):
        """최신 결과 파일들 찾기"""
        current_time = datetime.now()
        result_files = []
        
        # 최근 10분 내 생성된 파일들 찾기
        for pattern in ['performance_results_*.json', 'performance_test_*.csv']:
            for file_path in Path('.').glob(pattern):
                try:
                    file_time = datetime.fromtimestamp(file_path.stat().st_mtime)
                    if (current_time - file_time).total_seconds() < 600:  # 10분
                        result_files.append(str(file_path))
                except:
                    continue
        
        return result_files
    
    def run_scalability_test_suite(self, scenarios=None, client_counts=None, 
                                 custom_duration=None, stabilization_time=30):
        """확장성 테스트 스위트 실행"""
        self.test_start_time = datetime.now()
        
        # 기본값 설정
        if scenarios is None:
            scenarios = ['basic', 'stress']
        
        if client_counts is None:
            client_counts = [10, 25, 50, 100, 200, 300, 500]
        
        print(f"{'='*100}")
        print(f"CppMMO 서버 확장성 테스트 스위트")
        print(f"시작 시간: {self.test_start_time.strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"{'='*100}")
        print(f"🎯 테스트 시나리오: {', '.join(scenarios)}")
        print(f"👥 클라이언트 수: {client_counts}")
        print(f"⏱️ 안정화 대기시간: {stabilization_time}초")
        
        # 예상 소요 시간 계산
        total_tests = len(scenarios) * len(client_counts)
        avg_test_duration = custom_duration if custom_duration else 180  # 평균 3분
        estimated_time = total_tests * (avg_test_duration + stabilization_time) / 60  # 분 단위
        
        print(f"📊 총 테스트 수: {total_tests}")
        print(f"⏰ 예상 소요 시간: {estimated_time:.1f}분")
        
        # 서버 상태 확인
        if not self.check_server_status():
            print("❌ 서버에 연결할 수 없습니다. 서버가 실행 중인지 확인하세요.")
            return None
        
        print("✅ 서버 연결 확인됨")
        
        # 확인 받기
        response = input("\n테스트를 시작하시겠습니까? (y/N): ").strip().lower()
        if response != 'y':
            print("테스트가 취소되었습니다.")
            return None
        
        test_count = 0
        
        try:
            for scenario in scenarios:
                print(f"\n🎮 {scenario.upper()} 시나리오 테스트 시작")
                
                for client_count in client_counts:
                    test_count += 1
                    
                    print(f"\n[{test_count}/{total_tests}] 테스트 진행률: {test_count/total_tests*100:.1f}%")
                    
                    # 안정화 대기 (첫 번째 테스트 제외)
                    if test_count > 1:
                        self.wait_for_server_stabilization(stabilization_time)
                    
                    # 테스트 실행
                    result = self.run_performance_test(scenario, client_count, custom_duration)
                    
                    # 결과 저장
                    test_key = f"{scenario}_{client_count}"
                    self.test_results[test_key] = result
                    
                    # 실패한 경우 조치
                    if not result['success']:
                        print(f"⚠️ 테스트 실패: {test_key}")
                        
                        # 심각한 오류인 경우 중단 여부 확인
                        if result.get('timeout') or result.get('exception'):
                            response = input("계속 진행하시겠습니까? (y/N): ").strip().lower()
                            if response != 'y':
                                print("테스트가 중단되었습니다.")
                                break
                    else:
                        print(f"✅ 테스트 성공: {test_key}")
                
                # 시나리오 간 긴 휴식
                if scenario != scenarios[-1]:
                    print(f"\n💤 다음 시나리오까지 60초 대기...")
                    time.sleep(60)
        
        except KeyboardInterrupt:
            print("\n⛔ 사용자에 의해 테스트가 중단되었습니다.")
        except Exception as e:
            print(f"\n💥 테스트 실행 중 예외 발생: {e}")
        
        # 결과 요약
        self.generate_test_summary()
        
        return self.test_results
    
    def generate_test_summary(self):
        """테스트 결과 요약 생성"""
        end_time = datetime.now()
        total_duration = (end_time - self.test_start_time).total_seconds() / 60  # 분
        
        successful_tests = [r for r in self.test_results.values() if r['success']]
        failed_tests = [r for r in self.test_results.values() if not r['success']]
        
        print(f"\n{'='*100}")
        print(f"확장성 테스트 스위트 완료")
        print(f"{'='*100}")
        print(f"⏰ 총 소요 시간: {total_duration:.1f}분")
        print(f"📊 전체 테스트: {len(self.test_results)}")
        print(f"✅ 성공: {len(successful_tests)}")
        print(f"❌ 실패: {len(failed_tests)}")
        print(f"📈 성공률: {len(successful_tests)/len(self.test_results)*100:.1f}%")
        
        # 성공한 테스트 상세 정보
        if successful_tests:
            print(f"\n✅ 성공한 테스트:")
            for test in successful_tests:
                scenario = test['scenario']
                client_count = test['client_count']
                duration = test['test_duration']
                print(f"   {scenario}_{client_count}: {duration:.1f}초")
        
        # 실패한 테스트 상세 정보
        if failed_tests:
            print(f"\n❌ 실패한 테스트:")
            for test in failed_tests:
                scenario = test['scenario']
                client_count = test['client_count']
                error = test.get('error', 'Unknown error')[:100]
                print(f"   {scenario}_{client_count}: {error}")
        
        # 결과 파일 저장
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        summary_file = f"scalability_test_summary_{timestamp}.json"
        
        summary_data = {
            'test_start_time': self.test_start_time.isoformat(),
            'test_end_time': end_time.isoformat(),
            'total_duration_minutes': total_duration,
            'total_tests': len(self.test_results),
            'successful_tests': len(successful_tests),
            'failed_tests': len(failed_tests),
            'success_rate': len(successful_tests)/len(self.test_results)*100,
            'test_results': self.test_results
        }
        
        with open(summary_file, 'w', encoding='utf-8') as f:
            json.dump(summary_data, f, indent=2, ensure_ascii=False)
        
        print(f"\n📄 상세 결과가 {summary_file}에 저장되었습니다.")
        
        # 분석 실행 권장
        print(f"\n💡 다음 단계:")
        print(f"   python performance_analysis.py")
        print(f"   # 생성된 모든 성능 데이터를 분석하여 시각화 차트를 생성합니다.")

def main():
    parser = argparse.ArgumentParser(description='CppMMO 서버 확장성 테스트')
    parser.add_argument('--scenarios', '-s',
                        nargs='+',
                        choices=['basic', 'stress', 'extreme'],
                        default=['basic', 'stress'],
                        help='테스트할 시나리오')
    parser.add_argument('--clients', '-c',
                        nargs='+',
                        type=int,
                        default=[10, 25, 50, 100, 200, 300, 500],
                        help='테스트할 클라이언트 수 목록')
    parser.add_argument('--duration', '-d',
                        type=int,
                        help='각 테스트의 지속 시간(초)')
    parser.add_argument('--stabilization-time', '-w',
                        type=int,
                        default=30,
                        help='테스트 간 안정화 대기 시간(초)')
    parser.add_argument('--quick', '-q',
                        action='store_true',
                        help='빠른 테스트 (적은 클라이언트 수, 짧은 시간)')
    
    args = parser.parse_args()
    
    if args.quick:
        scenarios = ['basic']
        client_counts = [10, 25, 50]
        duration = 60  # 1분
        stabilization_time = 15  # 15초
    else:
        scenarios = args.scenarios
        client_counts = args.clients
        duration = args.duration
        stabilization_time = args.stabilization_time
    
    print("CppMMO 서버 확장성 테스트를 시작합니다.")
    print("\n주의사항:")
    print("1. CppMMO 서버가 localhost:8080에서 실행 중이어야 합니다")
    print("2. 테스트 중에는 다른 네트워크 집약적 작업을 피해주세요")
    print("3. 충분한 디스크 공간을 확보해주세요 (결과 파일용)")
    print("4. 테스트는 상당한 시간이 소요될 수 있습니다")
    
    runner = ScalabilityTestRunner()
    results = runner.run_scalability_test_suite(
        scenarios=scenarios,
        client_counts=client_counts,
        custom_duration=duration,
        stabilization_time=stabilization_time
    )
    
    if results:
        print(f"\n🎊 확장성 테스트가 완료되었습니다!")
        print(f"📊 이제 performance_analysis.py를 실행하여 결과를 분석하세요.")

if __name__ == "__main__":
    main()