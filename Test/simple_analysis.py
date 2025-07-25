#!/usr/bin/env python3
"""
Simple Performance Analysis without pandas
CppMMO 서버 성능 테스트 결과 간단 분석
"""
import json
import glob
import os
from datetime import datetime

def find_result_files():
    """최근 결과 파일들 찾기"""
    json_files = glob.glob('performance_results_*.json')
    csv_files = glob.glob('performance_test_*.csv')
    
    # 시간순 정렬
    json_files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    csv_files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    return json_files, csv_files

def analyze_json_results(json_files):
    """JSON 결과 파일 분석"""
    results = []
    
    for json_file in json_files:
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                
                client_count = data.get('client_count', 0)
                final_stats = data.get('final_stats', {})
                overall_score = data.get('overall_score', 0)
                
                # 처리량 데이터
                throughput = final_stats.get('throughput', {})
                packets_per_sec = throughput.get('packets_per_sec', 0)
                
                # 지연시간 데이터
                latency = final_stats.get('latency', {})
                avg_latency = latency.get('avg', 0)
                p95_latency = latency.get('p95', 0)
                
                # 시스템 리소스
                system = final_stats.get('system_resources', {})
                cpu_usage = system.get('cpu_usage', 0)
                memory_usage = system.get('memory_usage', 0)
                
                # 안정성
                errors = final_stats.get('errors', {})
                error_rate = errors.get('error_rate_percent', 0)
                
                # 연결 성공률
                connections = final_stats.get('connections', {})
                connected = connections.get('connected', 0)
                connection_success_rate = (connected / client_count * 100) if client_count > 0 else 0
                
                results.append({
                    'client_count': client_count,
                    'packets_per_sec': packets_per_sec,
                    'avg_latency_ms': avg_latency,
                    'p95_latency_ms': p95_latency,
                    'cpu_usage_percent': cpu_usage,
                    'memory_usage_percent': memory_usage,
                    'error_rate_percent': error_rate,
                    'connection_success_rate': connection_success_rate,
                    'overall_score': overall_score,
                    'file': json_file
                })
                
        except Exception as e:
            print(f"파일 읽기 오류 ({json_file}): {e}")
    
    return results

def print_performance_summary(results):
    """성능 요약 출력"""
    if not results:
        print("분석할 결과가 없습니다.")
        return
    
    # 클라이언트 수별로 정렬
    results.sort(key=lambda x: x['client_count'])
    
    print(f"\n{'='*100}")
    print(f"CppMMO 서버 성능 테스트 결과 요약")
    print(f"{'='*100}")
    print(f"분석된 테스트: {len(results)}개")
    print(f"클라이언트 수 범위: {results[0]['client_count']}명 ~ {results[-1]['client_count']}명")
    print(f"{'='*100}")
    
    # 헤더 출력
    print(f"{'클라이언트수':>8} {'처리량(pps)':>12} {'평균지연(ms)':>12} {'P95지연(ms)':>12} {'CPU(%)':>8} {'메모리(%)':>9} {'연결률(%)':>9} {'종합점수':>8} {'등급':>4}")
    print(f"{'-'*100}")
    
    for result in results:
        # 등급 계산
        score = result['overall_score']
        if score >= 90:
            grade = "S급"
        elif score >= 80:
            grade = "A급"
        elif score >= 70:
            grade = "B급"
        elif score >= 60:
            grade = "C급"
        else:
            grade = "D급"
        
        print(f"{result['client_count']:>8d} "
              f"{result['packets_per_sec']:>12.1f} "
              f"{result['avg_latency_ms']:>12.2f} "
              f"{result['p95_latency_ms']:>12.2f} "
              f"{result['cpu_usage_percent']:>8.1f} "
              f"{result['memory_usage_percent']:>9.1f} "
              f"{result['connection_success_rate']:>9.1f} "
              f"{result['overall_score']:>8.1f} "
              f"{grade:>4}")
    
    print(f"{'-'*100}")
    
    # 성능 분석
    print(f"\n📊 성능 분석:")
    
    # 최고 처리량
    max_throughput = max(results, key=lambda x: x['packets_per_sec'])
    print(f"최고 처리량: {max_throughput['client_count']}명에서 {max_throughput['packets_per_sec']:.1f} packets/sec")
    
    # 최저 지연시간
    min_latency = min(results, key=lambda x: x['p95_latency_ms'] if x['p95_latency_ms'] > 0 else float('inf'))
    if min_latency['p95_latency_ms'] > 0:
        print(f"최저 P95 지연시간: {min_latency['client_count']}명에서 {min_latency['p95_latency_ms']:.2f}ms")
    
    # CPU 100% 도달점
    cpu_100_results = [r for r in results if r['cpu_usage_percent'] >= 99.0]
    if cpu_100_results:
        first_cpu_100 = min(cpu_100_results, key=lambda x: x['client_count'])
        print(f"CPU 한계점: {first_cpu_100['client_count']}명에서 CPU {first_cpu_100['cpu_usage_percent']:.1f}%")
    
    # 권장 최대 클라이언트 수
    stable_results = [r for r in results if 
                     r['error_rate_percent'] < 1.0 and 
                     r['connection_success_rate'] > 95.0 and
                     r['cpu_usage_percent'] < 90.0]
    
    if stable_results:
        max_stable = max(stable_results, key=lambda x: x['client_count'])
        print(f"권장 최대 클라이언트: {max_stable['client_count']}명 (안정적 운영 기준)")
    
    # 성능 저하 시작점
    if len(results) >= 2:
        for i in range(1, len(results)):
            prev_score = results[i-1]['overall_score']
            curr_score = results[i]['overall_score']
            
            if prev_score - curr_score > 10:  # 10점 이상 감소
                print(f"성능 저하 시작: {results[i-1]['client_count']}명 → {results[i]['client_count']}명에서 점수 {prev_score:.1f} → {curr_score:.1f}")
                break
    
    print(f"\n💡 최적화 권장사항:")
    
    # CPU가 높은 경우
    high_cpu_results = [r for r in results if r['cpu_usage_percent'] > 80]
    if high_cpu_results:
        print(f"- CPU 최적화 필요: {len(high_cpu_results)}개 테스트에서 CPU > 80%")
        print(f"  권장: 프로파일링, 알고리즘 최적화, 멀티스레딩 개선")
    
    # 지연시간이 높은 경우
    high_latency_results = [r for r in results if r['p95_latency_ms'] > 100]
    if high_latency_results:
        print(f"- 지연시간 개선 필요: {len(high_latency_results)}개 테스트에서 P95 지연시간 > 100ms")
        print(f"  권장: 네트워크 버퍼 튜닝, 응답 시간 최적화")
    
    # 메모리 사용률이 높은 경우
    high_memory_results = [r for r in results if r['memory_usage_percent'] > 70]
    if high_memory_results:
        print(f"- 메모리 최적화 필요: {len(high_memory_results)}개 테스트에서 메모리 > 70%")
        print(f"  권장: 메모리 풀 도입, 불필요한 할당 최소화")

def main():
    print("CppMMO 서버 성능 분석 (Simple Version)")
    
    json_files, csv_files = find_result_files()
    
    if not json_files:
        print("분석할 성능 결과 파일이 없습니다.")
        print("먼저 성능 테스트를 실행하세요:")
        print("python comprehensive_performance_test.py --scenario basic --clients 200")
        return
    
    print(f"발견된 결과 파일: {len(json_files)}개")
    
    # JSON 파일 분석
    results = analyze_json_results(json_files)
    
    # 요약 출력
    print_performance_summary(results)
    
    # 결과를 텍스트 파일로 저장
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = f"performance_summary_{timestamp}.txt"
    
    with open(report_file, 'w', encoding='utf-8') as f:
        f.write("CppMMO 서버 성능 테스트 결과 요약\n")
        f.write(f"생성 시간: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("="*100 + "\n")
        
        f.write(f"분석된 테스트: {len(results)}개\n")
        if results:
            f.write(f"클라이언트 수 범위: {results[0]['client_count']}명 ~ {results[-1]['client_count']}명\n")
        f.write("="*100 + "\n\n")
        
        # 상세 데이터
        f.write("상세 결과:\n")
        f.write(f"{'클라이언트수':>8} {'처리량(pps)':>12} {'평균지연(ms)':>12} {'P95지연(ms)':>12} {'CPU(%)':>8} {'메모리(%)':>9} {'연결률(%)':>9} {'종합점수':>8}\n")
        f.write("-"*100 + "\n")
        
        for result in results:
            f.write(f"{result['client_count']:>8d} "
                  f"{result['packets_per_sec']:>12.1f} "
                  f"{result['avg_latency_ms']:>12.2f} "
                  f"{result['p95_latency_ms']:>12.2f} "
                  f"{result['cpu_usage_percent']:>8.1f} "
                  f"{result['memory_usage_percent']:>9.1f} "
                  f"{result['connection_success_rate']:>9.1f} "
                  f"{result['overall_score']:>8.1f}\n")
    
    print(f"\n📄 상세 리포트가 {report_file}에 저장되었습니다.")

if __name__ == "__main__":
    main()