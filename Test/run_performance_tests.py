#!/usr/bin/env python3
"""
CppMMO 서버 성능 테스트 실행 통합 스크립트
여러 성능 테스트를 순차적으로 실행하고 결과를 통합 분석합니다.
"""
import subprocess
import sys
import time
import json
import os
from datetime import datetime
import argparse

class PerformanceTestRunner:
    def __init__(self):
        self.results = {}
        self.test_start_time = None
        
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
    
    def run_system_monitor_background(self, duration):
        """백그라운드에서 시스템 모니터 실행"""
        print("백그라운드 시스템 모니터링 시작...")
        
        cmd = [sys.executable, 'system_monitor.py', '--duration', str(duration)]
        process = subprocess.Popen(cmd, 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE,
                                 universal_newlines=True)
        return process
    
    def run_comprehensive_test(self, scenario):
        """종합 성능 테스트 실행"""
        print(f"\n{'='*80}")
        print(f"종합 성능 테스트 시작: {scenario.upper()} 시나리오")
        print(f"{'='*80}")
        
        cmd = [sys.executable, 'comprehensive_performance_test.py', '--scenario', scenario]
        
        try:
            result = subprocess.run(cmd, 
                                  capture_output=True, 
                                  text=True, 
                                  timeout=3600)  # 1시간 타임아웃
            
            if result.returncode == 0:
                print(f"✅ {scenario} 시나리오 테스트 완료")
                return True, result.stdout, result.stderr
            else:
                print(f"❌ {scenario} 시나리오 테스트 실패")
                print(f"오류: {result.stderr}")
                return False, result.stdout, result.stderr
                
        except subprocess.TimeoutExpired:
            print(f"⏰ {scenario} 시나리오 테스트 타임아웃")
            return False, "", "Test timeout"
        except Exception as e:
            print(f"💥 {scenario} 시나리오 테스트 예외: {e}")
            return False, "", str(e)
    
    def run_specific_load_test(self, test_script, test_name):
        """특정 부하 테스트 실행"""
        print(f"\n{'='*60}")
        print(f"{test_name} 실행 중...")
        print(f"{'='*60}")
        
        cmd = [sys.executable, test_script]
        
        try:
            result = subprocess.run(cmd,
                                  capture_output=True,
                                  text=True,
                                  timeout=1800)  # 30분 타임아웃
            
            if result.returncode == 0:
                print(f"✅ {test_name} 완료")
                return True, result.stdout, result.stderr
            else:
                print(f"❌ {test_name} 실패")
                return False, result.stdout, result.stderr
                
        except subprocess.TimeoutExpired:
            print(f"⏰ {test_name} 타임아웃")
            return False, "", "Test timeout"
        except Exception as e:
            print(f"💥 {test_name} 예외: {e}")
            return False, "", str(e)
    
    def analyze_result_files(self):
        """생성된 결과 파일들 분석"""
        result_files = {
            'performance_results': [],
            'csv_files': [],
            'system_monitor': [],
            'other_logs': []
        }
        
        # 현재 디렉토리의 파일들 스캔
        current_time = datetime.now()
        for filename in os.listdir('.'):
            # 최근 1시간 내 생성된 파일만 고려
            try:
                file_time = datetime.fromtimestamp(os.path.getmtime(filename))
                if (current_time - file_time).total_seconds() > 3600:
                    continue
            except:
                continue
            
            if filename.startswith('performance_results_') and filename.endswith('.json'):
                result_files['performance_results'].append(filename)
            elif filename.endswith('.csv'):
                if 'system_monitor' in filename:
                    result_files['system_monitor'].append(filename)
                else:
                    result_files['csv_files'].append(filename)
            elif filename.endswith('.log'):
                result_files['other_logs'].append(filename)
        
        return result_files
    
    def generate_combined_report(self, test_results, result_files):
        """통합 리포트 생성"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_filename = f"comprehensive_test_report_{timestamp}.json"
        
        # 성능 테스트 결과 파일들 읽기
        performance_data = {}
        for perf_file in result_files['performance_results']:
            try:
                with open(perf_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    scenario = data.get('scenario', 'unknown')
                    performance_data[scenario] = data
            except Exception as e:
                print(f"결과 파일 읽기 오류 ({perf_file}): {e}")
        
        # 통합 리포트 데이터
        combined_report = {
            'test_execution_info': {
                'start_time': self.test_start_time.isoformat() if self.test_start_time else None,
                'end_time': datetime.now().isoformat(),
                'total_duration_minutes': (datetime.now() - self.test_start_time).total_seconds() / 60 if self.test_start_time else 0
            },
            'test_results': test_results,
            'performance_data': performance_data,
            'result_files': result_files,
            'summary': self.generate_test_summary(test_results, performance_data)
        }
        
        # JSON 파일로 저장
        with open(report_filename, 'w', encoding='utf-8') as f:
            json.dump(combined_report, f, indent=2, ensure_ascii=False)
        
        print(f"\n📊 통합 리포트가 {report_filename}에 저장되었습니다.")
        
        # 콘솔에 요약 출력
        self.print_test_summary(combined_report['summary'])
        
        return report_filename
    
    def generate_test_summary(self, test_results, performance_data):
        """테스트 요약 생성"""
        summary = {
            'tests_executed': len(test_results),
            'tests_passed': sum(1 for result in test_results.values() if result['success']),
            'tests_failed': sum(1 for result in test_results.values() if not result['success']),
            'scenarios_tested': list(performance_data.keys()),
            'overall_performance': {}
        }
        
        # 성능 데이터 요약
        if performance_data:
            all_scores = [data.get('overall_score', 0) for data in performance_data.values() if 'overall_score' in data]
            if all_scores:
                summary['overall_performance'] = {
                    'average_score': sum(all_scores) / len(all_scores),
                    'best_score': max(all_scores),
                    'worst_score': min(all_scores),
                    'scores_by_scenario': {scenario: data.get('overall_score', 0) 
                                         for scenario, data in performance_data.items()}
                }
        
        return summary
    
    def print_test_summary(self, summary):
        """테스트 요약 콘솔 출력"""
        print(f"\n{'='*100}")
        print(f"CppMMO 서버 성능 테스트 통합 결과")
        print(f"{'='*100}")
        
        print(f"📋 테스트 실행 현황:")
        print(f"  총 테스트: {summary['tests_executed']}")
        print(f"  성공: {summary['tests_passed']} | 실패: {summary['tests_failed']}")
        print(f"  성공률: {summary['tests_passed'] / summary['tests_executed'] * 100:.1f}%")
        
        if summary['scenarios_tested']:
            print(f"\n🎯 테스트된 시나리오:")
            for scenario in summary['scenarios_tested']:
                print(f"  - {scenario.upper()}")
        
        if summary['overall_performance']:
            perf = summary['overall_performance']
            print(f"\n🏆 종합 성능 점수:")
            print(f"  평균 점수: {perf['average_score']:.1f}/100")
            print(f"  최고 점수: {perf['best_score']:.1f}/100")
            print(f"  최저 점수: {perf['worst_score']:.1f}/100")
            
            print(f"\n📊 시나리오별 점수:")
            for scenario, score in perf['scores_by_scenario'].items():
                if score >= 90:
                    grade = "🏅 S급"
                elif score >= 80:
                    grade = "🥈 A급"
                elif score >= 70:
                    grade = "🥉 B급"
                elif score >= 60:
                    grade = "⚠️ C급"
                else:
                    grade = "❌ D급"
                
                print(f"  {scenario.upper()}: {score:.1f}/100 {grade}")
        
        print(f"\n💡 종합 평가:")
        if summary['tests_failed'] == 0:
            if summary.get('overall_performance', {}).get('average_score', 0) >= 80:
                print("  🎉 우수 - 모든 테스트 통과, 높은 성능 점수")
            else:
                print("  ✅ 양호 - 모든 테스트 통과, 성능 개선 여지 있음")
        else:
            print("  ⚠️ 개선 필요 - 일부 테스트 실패 또는 성능 이슈")
        
        print(f"{'='*100}")
    
    def run_all_tests(self, scenarios=None, include_load_tests=True):
        """모든 성능 테스트 실행"""
        self.test_start_time = datetime.now()
        test_results = {}
        
        print(f"{'='*100}")
        print(f"CppMMO 서버 성능 테스트 통합 실행")
        print(f"시작 시간: {self.test_start_time.strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"{'='*100}")
        
        # 서버 상태 확인
        if not self.check_server_status():
            print("❌ 서버에 연결할 수 없습니다. 서버가 실행 중인지 확인하세요.")
            return
        
        print("✅ 서버 연결 확인됨")
        
        # 시나리오 설정
        if scenarios is None:
            scenarios = ['basic', 'stress']  # extreme은 선택적
        
        # 전체 테스트 시간 추정
        scenario_durations = {'basic': 2, 'stress': 5, 'extreme': 10}  # 분 단위
        estimated_time = sum(scenario_durations.get(s, 5) for s in scenarios)
        if include_load_tests:
            estimated_time += 10  # 추가 부하 테스트
        
        print(f"📅 예상 소요 시간: 약 {estimated_time}분")
        print(f"🔄 테스트 시나리오: {', '.join(s.upper() for s in scenarios)}")
        
        # 백그라운드 시스템 모니터링 시작
        total_duration = estimated_time * 60 + 120  # 여유 시간 2분 추가
        monitor_process = self.run_system_monitor_background(total_duration)
        
        time.sleep(3)  # 모니터링 안정화 대기
        
        try:
            # 1. 종합 성능 테스트 실행
            for scenario in scenarios:
                print(f"\n⏳ {scenario.upper()} 시나리오 테스트 준비 중...")
                time.sleep(2)  # 서버 안정화
                
                success, stdout, stderr = self.run_comprehensive_test(scenario)
                test_results[f'comprehensive_{scenario}'] = {
                    'success': success,
                    'stdout': stdout[:1000] if stdout else "",  # 길이 제한
                    'stderr': stderr[:500] if stderr else "",
                    'scenario': scenario
                }
                
                if success:
                    print(f"✅ {scenario.upper()} 시나리오 완료")
                else:
                    print(f"❌ {scenario.upper()} 시나리오 실패")
                
                # 시나리오 간 휴식
                if scenario != scenarios[-1]:
                    print("💤 다음 테스트까지 30초 대기...")
                    time.sleep(30)
            
            # 2. 추가 부하 테스트 (선택적)
            if include_load_tests:
                load_tests = [
                    ('movement_load_test.py', '실시간 이동 부하 테스트'),
                    ('no_auth_stress_test.py', '인증 우회 부하 테스트')
                ]
                
                for script, name in load_tests:
                    if os.path.exists(script):
                        print(f"\n💤 부하 테스트 준비를 위해 60초 대기...")
                        time.sleep(60)
                        
                        success, stdout, stderr = self.run_specific_load_test(script, name)
                        test_results[script.replace('.py', '')] = {
                            'success': success,
                            'stdout': stdout[:1000] if stdout else "",
                            'stderr': stderr[:500] if stderr else "",
                            'test_name': name
                        }
            
        except KeyboardInterrupt:
            print("\n⛔ 사용자에 의해 테스트가 중단되었습니다.")
        except Exception as e:
            print(f"\n💥 테스트 실행 중 예외 발생: {e}")
        finally:
            # 시스템 모니터 프로세스 종료
            if monitor_process and monitor_process.poll() is None:
                print("\n📊 백그라운드 시스템 모니터링 종료 중...")
                monitor_process.terminate()
                try:
                    monitor_process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    monitor_process.kill()
        
        # 결과 파일 분석
        print("\n📁 결과 파일 분석 중...")
        result_files = self.analyze_result_files()
        
        # 통합 리포트 생성
        print("\n📋 통합 리포트 생성 중...")
        report_file = self.generate_combined_report(test_results, result_files)
        
        print(f"\n🎊 모든 성능 테스트가 완료되었습니다!")
        print(f"📊 상세 결과는 {report_file}를 확인하세요.")
        
        return test_results, result_files

def main():
    parser = argparse.ArgumentParser(description='CppMMO 서버 성능 테스트 통합 실행')
    parser.add_argument('--scenarios', '-s', 
                        nargs='+',
                        choices=['basic', 'stress', 'extreme'],
                        default=['basic', 'stress'],
                        help='실행할 테스트 시나리오')
    parser.add_argument('--no-load-tests', 
                        action='store_true',
                        help='추가 부하 테스트 제외')
    parser.add_argument('--quick', '-q',
                        action='store_true',
                        help='빠른 테스트 (basic 시나리오만)')
    
    args = parser.parse_args()
    
    if args.quick:
        scenarios = ['basic']
        include_load_tests = False
    else:
        scenarios = args.scenarios
        include_load_tests = not args.no_load_tests
    
    print("CppMMO 서버 성능 테스트를 시작합니다...")
    print("실행 전에 다음을 확인해주세요:")
    print("1. CppMMO 서버가 localhost:8080에서 실행 중인지")
    print("2. 테스트 중 다른 애플리케이션을 최소화할 것")
    print("3. 충분한 디스크 공간이 있는지 (결과 파일용)")
    
    response = input("\n준비가 완료되었나요? (y/N): ").strip().lower()
    if response != 'y':
        print("테스트가 취소되었습니다.")
        return
    
    runner = PerformanceTestRunner()
    runner.run_all_tests(scenarios, include_load_tests)

if __name__ == "__main__":
    main()