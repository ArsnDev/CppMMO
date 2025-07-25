#!/usr/bin/env python3
"""
CSV 기반 성능 분석
CppMMO 서버 성능 테스트 CSV 결과 분석
"""
import csv
import glob
import os
from datetime import datetime

def load_csv_files():
    """CSV 파일들 로드"""
    csv_files = glob.glob('performance_test_*.csv')
    csv_files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    all_data = []
    
    for csv_file in csv_files:
        try:
            with open(csv_file, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f)
                file_data = []
                
                for row in reader:
                    try:
                        # 숫자 변환
                        processed_row = {}
                        for key, value in row.items():
                            try:
                                # 정수형 필드
                                if key in ['elapsed_time', 'clients_connected', 'clients_in_zone', 
                                          'packets_sent_total', 'send_errors', 'receive_errors', 'connection_failures']:
                                    processed_row[key] = int(float(value))
                                # 실수형 필드
                                else:
                                    processed_row[key] = float(value)
                            except ValueError:
                                processed_row[key] = value
                        
                        file_data.append(processed_row)
                    except Exception as e:
                        continue
                
                if file_data:
                    # 파일별 최종 성능 데이터 추출 (가장 마지막 행)
                    final_row = file_data[-1]
                    final_row['source_file'] = csv_file
                    all_data.append(final_row)
                    
                    print(f"로드: {csv_file} - {len(file_data)}행, 최종 {final_row['clients_connected']}명 클라이언트")
                    
        except Exception as e:
            print(f"CSV 로드 오류 ({csv_file}): {e}")
    
    return all_data

def calculate_performance_score(data):
    """성능 점수 계산"""
    try:
        clients = data['clients_in_zone']
        packets_per_sec = data['packets_per_sec']
        cpu_usage = data['cpu_usage']
        error_rate = data['error_rate_percent']
        connection_success_rate = (data['clients_connected'] / clients * 100) if clients > 0 else 0
        
        # 처리량 점수 (목표 대비)
        expected_packets = clients * 20  # 20fps 가정
        throughput_score = min(100, (packets_per_sec / expected_packets * 100)) if expected_packets > 0 else 0
        
        # CPU 효율성 점수
        cpu_score = max(0, 100 - cpu_usage)
        
        # 안정성 점수
        stability_score = max(0, 100 - error_rate * 10)
        
        # 연결 점수
        connection_score = min(100, connection_success_rate)
        
        # 종합 점수 (가중평균)
        overall_score = (throughput_score * 0.4 + cpu_score * 0.3 + 
                        stability_score * 0.2 + connection_score * 0.1)
        
        return overall_score
        
    except Exception as e:
        return 0

def print_csv_analysis(data_list):
    """CSV 분석 결과 출력"""
    if not data_list:
        print("분석할 데이터가 없습니다.")
        return
    
    # 클라이언트 수별 정렬
    data_list.sort(key=lambda x: x['clients_connected'])
    
    print(f"\n{'='*120}")
    print(f"CppMMO 서버 성능 테스트 CSV 분석 결과")
    print(f"{'='*120}")
    print(f"분석된 테스트: {len(data_list)}개")
    if data_list:
        print(f"클라이언트 수 범위: {data_list[0]['clients_connected']}명 ~ {data_list[-1]['clients_connected']}명")
    print(f"{'='*120}")
    
    # 성능 점수 계산
    for data in data_list:
        data['calculated_score'] = calculate_performance_score(data)
    
    # 헤더 출력
    print(f"{'클라이언트':>8} {'처리량':>10} {'대역폭송신':>10} {'대역폭수신':>10} {'CPU':>6} {'메모리':>6} {'오류율':>8} {'성능점수':>8} {'등급':>4}")
    print(f"{'수':>8} {'(pps)':>10} {'(Mbps)':>10} {'(Mbps)':>10} {'(%)':>6} {'(%)':>6} {'(%)':>8} {'':>8} {'':>4}")
    print(f"{'-'*120}")
    
    for data in data_list:
        # 등급 계산
        score = data['calculated_score']
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
        
        print(f"{data['clients_connected']:>8d} "
              f"{data['packets_per_sec']:>10.1f} "
              f"{data['mbps_sent']:>10.2f} "
              f"{data['mbps_received']:>10.2f} "
              f"{data['cpu_usage']:>6.1f} "
              f"{data['memory_usage']:>6.1f} "
              f"{data['error_rate_percent']:>8.2f} "
              f"{score:>8.1f} "
              f"{grade:>4}")
    
    print(f"{'-'*120}")
    
    # 성능 분석 및 인사이트
    print(f"\n[성능 분석 결과]")
    
    # 최고 처리량
    max_throughput = max(data_list, key=lambda x: x['packets_per_sec'])
    print(f"- 최고 처리량: {max_throughput['clients_connected']}명에서 {max_throughput['packets_per_sec']:.1f} packets/sec")
    
    # CPU 한계점
    cpu_limit = next((d for d in data_list if d['cpu_usage'] >= 90), None)
    if cpu_limit:
        print(f"- CPU 한계점: {cpu_limit['clients_connected']}명에서 CPU {cpu_limit['cpu_usage']:.1f}%")
    
    # 효율성 분석
    efficiency_data = []
    for data in data_list:
        if data['clients_connected'] > 0:
            efficiency = data['packets_per_sec'] / data['clients_connected']
            efficiency_data.append((data['clients_connected'], efficiency))
    
    if len(efficiency_data) > 1:
        print(f"- 클라이언트당 처리량:")
        for clients, eff in efficiency_data:
            print(f"  - {clients:3d}명: {eff:6.2f} packets/sec/client")
    
    # 확장성 패턴 분석
    if len(data_list) >= 3:
        print(f"\n[확장성 패턴 분석]")
        
        # 처리량 증가 패턴
        throughput_increases = []
        for i in range(1, len(data_list)):
            prev_tps = data_list[i-1]['packets_per_sec']
            curr_tps = data_list[i]['packets_per_sec']
            increase_rate = ((curr_tps - prev_tps) / prev_tps * 100) if prev_tps > 0 else 0
            throughput_increases.append(increase_rate)
            
            prev_clients = data_list[i-1]['clients_connected']
            curr_clients = data_list[i]['clients_connected']
            print(f"  {prev_clients}명 → {curr_clients}명: 처리량 {increase_rate:+.1f}% 변화")
        
        # 성능 저하 구간 식별
        declining_points = [(i, rate) for i, rate in enumerate(throughput_increases) if rate < -5]
        if declining_points:
            print(f"\n[성능 저하 구간]")
            for i, rate in declining_points:
                prev_clients = data_list[i]['clients_connected']
                curr_clients = data_list[i+1]['clients_connected']
                print(f"  {prev_clients}명 → {curr_clients}명: {rate:.1f}% 감소")
    
    # 권장사항
    print(f"\n[최적화 권장사항]")
    
    # CPU 과부하 체크
    high_cpu_tests = [d for d in data_list if d['cpu_usage'] > 80]
    if high_cpu_tests:
        min_cpu_clients = min(high_cpu_tests, key=lambda x: x['clients_connected'])['clients_connected']
        print(f"- CPU 최적화 필요: {min_cpu_clients}명부터 CPU 사용률 > 80%")
        print(f"  권장: 프로파일링, 알고리즘 최적화, 병렬 처리 개선")
    
    # 메모리 사용량 체크
    high_memory_tests = [d for d in data_list if d['memory_usage'] > 70]
    if high_memory_tests:
        min_memory_clients = min(high_memory_tests, key=lambda x: x['clients_connected'])['clients_connected']
        print(f"- 메모리 최적화 필요: {min_memory_clients}명부터 메모리 사용률 > 70%")
        print(f"  권장: 메모리 풀, 객체 재사용, 캐시 최적화")
    
    # 처리량 효율성 저하 체크
    if len(data_list) >= 2:
        first_efficiency = data_list[0]['packets_per_sec'] / data_list[0]['clients_connected']
        last_efficiency = data_list[-1]['packets_per_sec'] / data_list[-1]['clients_connected']
        efficiency_drop = (first_efficiency - last_efficiency) / first_efficiency * 100
        
        if efficiency_drop > 20:
            print(f"- 확장성 개선 필요: 클라이언트당 효율성 {efficiency_drop:.1f}% 감소")
            print(f"  권장: 아키텍처 검토, 부하 분산, 비동기 처리 개선")
    
    # 권장 운영 구간
    stable_tests = [d for d in data_list if 
                   d['cpu_usage'] < 80 and 
                   d['error_rate_percent'] == 0 and
                   d['calculated_score'] > 70]
    
    if stable_tests:
        max_stable_clients = max(stable_tests, key=lambda x: x['clients_connected'])['clients_connected']
        print(f"- 권장 최대 운영 클라이언트: {max_stable_clients}명")
        print(f"  기준: CPU < 80%, 오류율 = 0%, 성능점수 > 70점")

def save_analysis_report(data_list):
    """분석 리포트 저장"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = f"csv_analysis_report_{timestamp}.txt"
    
    with open(report_file, 'w', encoding='utf-8') as f:
        f.write("CppMMO 서버 성능 테스트 CSV 분석 리포트\n")
        f.write(f"생성 시간: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("="*120 + "\n\n")
        
        f.write(f"분석된 테스트: {len(data_list)}개\n")
        if data_list:
            f.write(f"클라이언트 수 범위: {data_list[0]['clients_connected']}명 ~ {data_list[-1]['clients_connected']}명\n")
        f.write("\n상세 데이터:\n")
        f.write(f"{'클라이언트수':>8} {'처리량(pps)':>12} {'CPU(%)':>8} {'메모리(%)':>9} {'성능점수':>8} {'파일명':>30}\n")
        f.write("-"*120 + "\n")
        
        for data in data_list:
            score = calculate_performance_score(data)
            filename = os.path.basename(data['source_file'])
            f.write(f"{data['clients_connected']:>8d} "
                  f"{data['packets_per_sec']:>12.1f} "
                  f"{data['cpu_usage']:>8.1f} "
                  f"{data['memory_usage']:>9.1f} "
                  f"{score:>8.1f} "
                  f"{filename:>30}\n")
    
    print(f"\n[리포트] 상세 리포트가 {report_file}에 저장되었습니다.")

def main():
    print("CppMMO 서버 성능 CSV 분석")
    
    # CSV 파일 로드
    data_list = load_csv_files()
    
    if not data_list:
        print("분석할 CSV 파일이 없습니다.")
        return
    
    # 분석 결과 출력
    print_csv_analysis(data_list)
    
    # 리포트 저장
    save_analysis_report(data_list)

if __name__ == "__main__":
    main()