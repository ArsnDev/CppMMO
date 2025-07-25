#!/usr/bin/env python3
"""
High Load Performance Test - 200~1000 clients
Windows console compatible version
"""
import subprocess
import sys
import time
import os
from datetime import datetime

def run_single_test(client_count, scenario='basic', duration=180):
    """단일 성능 테스트 실행"""
    print(f"\n{'='*60}")
    print(f"테스트 시작: {client_count}명 클라이언트")
    print(f"시나리오: {scenario}, 지속시간: {duration}초")
    print(f"{'='*60}")
    
    cmd = [
        sys.executable,
        'comprehensive_performance_test.py',
        '--scenario', scenario,
        '--clients', str(client_count),
        '--duration', str(duration)
    ]
    
    start_time = time.time()
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=duration + 120  # 2분 여유시간
        )
        
        end_time = time.time()
        test_duration = end_time - start_time
        
        if result.returncode == 0:
            print(f"SUCCESS: {client_count}명 테스트 완료 ({test_duration:.1f}초)")
            return True
        else:
            print(f"FAILED: {client_count}명 테스트 실패")
            print(f"Error: {result.stderr[:500]}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"TIMEOUT: {client_count}명 테스트 타임아웃")
        return False
    except Exception as e:
        print(f"EXCEPTION: {client_count}명 테스트 예외 - {e}")
        return False

def main():
    client_counts = [200, 400, 600, 800, 1000]
    scenario = 'basic'
    duration = 180  # 3분
    stabilization_time = 45  # 45초 대기
    
    print("High Load Performance Test")
    print(f"Client counts: {client_counts}")
    print(f"Duration per test: {duration} seconds")
    print(f"Stabilization time: {stabilization_time} seconds")
    print(f"Total estimated time: {len(client_counts) * (duration + stabilization_time) / 60:.1f} minutes")
    
    # 서버 연결 확인
    import socket
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        result = sock.connect_ex(('localhost', 8080))
        sock.close()
        if result != 0:
            print("ERROR: Cannot connect to server on localhost:8080")
            return
    except Exception:
        print("ERROR: Cannot connect to server")
        return
    
    print("Server connection confirmed")
    
    print("\nStarting high load test automatically...")
    
    results = {}
    
    for i, client_count in enumerate(client_counts):
        print(f"\n[{i+1}/{len(client_counts)}] Progress: {(i+1)/len(client_counts)*100:.1f}%")
        
        # 안정화 대기 (첫 번째 테스트 제외)
        if i > 0:
            print(f"Waiting {stabilization_time} seconds for server stabilization...")
            for remaining in range(stabilization_time, 0, -1):
                if remaining % 10 == 0 or remaining <= 5:
                    print(f"  {remaining} seconds remaining...")
                time.sleep(1)
        
        # 테스트 실행
        success = run_single_test(client_count, scenario, duration)
        results[client_count] = success
        
        if not success:
            print("Test failed. Continuing with next test...")
    
    # 결과 요약
    print(f"\n{'='*60}")
    print("High Load Test Results")
    print(f"{'='*60}")
    
    for client_count, success in results.items():
        status = "SUCCESS" if success else "FAILED"
        print(f"{client_count:4d} clients: {status}")
    
    successful_tests = sum(1 for success in results.values() if success)
    print(f"\nSuccess rate: {successful_tests}/{len(results)} ({successful_tests/len(results)*100:.1f}%)")
    
    # 분석 실행 제안
    if successful_tests > 0:
        print(f"\nNext step: Run analysis")
        print(f"python performance_analysis.py")

if __name__ == "__main__":
    main()