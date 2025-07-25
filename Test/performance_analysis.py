#!/usr/bin/env python3
"""
CppMMO 서버 성능 분석 및 시각화 도구
pandas와 matplotlib을 활용하여 인원수별 성능 차이를 분석합니다.
"""
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np
import json
import glob
import os
from datetime import datetime
import seaborn as sns
from pathlib import Path
import argparse

# 한글 폰트 설정
plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

class PerformanceAnalyzer:
    def __init__(self, data_directory='.'):
        self.data_directory = data_directory
        self.performance_data = {}
        self.csv_data = {}
        
    def load_performance_data(self):
        """성능 테스트 결과 JSON 파일들을 로드"""
        json_files = glob.glob(os.path.join(self.data_directory, 'performance_results_*.json'))
        
        print(f"발견된 성능 결과 파일: {len(json_files)}개")
        
        for json_file in json_files:
            try:
                with open(json_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    scenario = data.get('scenario', 'unknown')
                    client_count = data.get('client_count', 0)
                    
                    key = f"{scenario}_{client_count}"
                    self.performance_data[key] = data
                    
                    print(f"로드: {json_file} - {scenario} 시나리오, {client_count}명")
                    
            except Exception as e:
                print(f"파일 로드 오류 ({json_file}): {e}")
    
    def load_csv_data(self):
        """성능 테스트 CSV 파일들을 로드"""
        csv_files = glob.glob(os.path.join(self.data_directory, 'performance_test_*.csv'))
        
        print(f"발견된 CSV 파일: {len(csv_files)}개")
        
        for csv_file in csv_files:
            try:
                df = pd.read_csv(csv_file)
                filename = os.path.basename(csv_file)
                self.csv_data[filename] = df
                
                print(f"로드: {filename} - {len(df)} 행")
                
            except Exception as e:
                print(f"CSV 로드 오류 ({csv_file}): {e}")
    
    def create_client_count_scenarios(self):
        """다양한 클라이언트 수로 테스트 시나리오 생성"""
        client_counts = [10, 25, 50, 100, 200, 300, 500]
        scenarios = ['basic', 'stress']
        
        test_configs = []
        for scenario in scenarios:
            for client_count in client_counts:
                config = {
                    'scenario': scenario,
                    'clients': client_count,
                    'duration': 120 if scenario == 'basic' else 180,  # 기본 2분, 스트레스 3분
                    'movement_interval': 0.05 if scenario == 'basic' else 0.033,
                    'chat_interval': 10.0 if scenario == 'basic' else 5.0
                }
                test_configs.append(config)
        
        return test_configs
    
    def extract_performance_metrics(self):
        """성능 데이터에서 주요 메트릭 추출"""
        metrics = []
        
        for key, data in self.performance_data.items():
            try:
                scenario = data.get('scenario', 'unknown')
                client_count = data.get('client_count', 0)
                final_stats = data.get('final_stats', {})
                
                # 처리량 지표
                throughput = final_stats.get('throughput', {})
                packets_per_sec = throughput.get('packets_per_sec', 0)
                mbps_sent = throughput.get('mbps_sent', 0)
                mbps_received = throughput.get('mbps_received', 0)
                
                # 지연시간 지표
                latency = final_stats.get('latency', {})
                avg_latency = latency.get('avg', 0)
                p95_latency = latency.get('p95', 0)
                p99_latency = latency.get('p99', 0)
                
                # RTT 지표
                rtt = final_stats.get('rtt', {})
                avg_rtt = rtt.get('avg', 0)
                
                # 시스템 리소스
                system = final_stats.get('system_resources', {})
                cpu_usage = system.get('cpu_usage', 0)
                memory_usage = system.get('memory_usage', 0)
                
                # 안정성 지표
                errors = final_stats.get('errors', {})
                error_rate = errors.get('error_rate_percent', 0)
                
                # 연결 지표
                connections = final_stats.get('connections', {})
                connected_clients = connections.get('connected', 0)
                in_zone_clients = connections.get('in_zone', 0)
                
                # 종합 점수
                overall_score = data.get('overall_score', 0)
                
                metric = {
                    'scenario': scenario,
                    'client_count': client_count,
                    'packets_per_sec': packets_per_sec,
                    'mbps_sent': mbps_sent,
                    'mbps_received': mbps_received,
                    'avg_latency_ms': avg_latency,
                    'p95_latency_ms': p95_latency,
                    'p99_latency_ms': p99_latency,
                    'avg_rtt_ms': avg_rtt,
                    'cpu_usage_percent': cpu_usage,
                    'memory_usage_percent': memory_usage,
                    'error_rate_percent': error_rate,
                    'connected_clients': connected_clients,
                    'in_zone_clients': in_zone_clients,
                    'connection_success_rate': (connected_clients / client_count * 100) if client_count > 0 else 0,
                    'overall_score': overall_score,
                    'timestamp': data.get('timestamp', ''),
                    'test_duration': data.get('test_duration', 0)
                }
                
                metrics.append(metric)
                
            except Exception as e:
                print(f"메트릭 추출 오류 ({key}): {e}")
        
        return pd.DataFrame(metrics)
    
    def create_scalability_charts(self, df, output_dir='performance_charts'):
        """인원수별 확장성 차트 생성"""
        os.makedirs(output_dir, exist_ok=True)
        
        # 시나리오별 색상 정의
        colors = {'basic': '#2E86AB', 'stress': '#F24236', 'extreme': '#F18F01'}
        
        # 1. 처리량 vs 클라이언트 수
        plt.figure(figsize=(15, 10))
        
        plt.subplot(2, 3, 1)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['packets_per_sec'], 
                    marker='o', label=f'{scenario.title()}', color=colors.get(scenario, '#333333'), linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Packets per Second')
        plt.title('Throughput Scalability')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # 2. 지연시간 vs 클라이언트 수
        plt.subplot(2, 3, 2)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['p95_latency_ms'], 
                    marker='s', label=f'{scenario.title()} P95', color=colors.get(scenario, '#333333'), linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('P95 Latency (ms)')
        plt.title('Latency Scalability')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.yscale('log')  # 로그 스케일로 변경
        
        # 3. CPU 사용률 vs 클라이언트 수
        plt.subplot(2, 3, 3)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['cpu_usage_percent'], 
                    marker='^', label=f'{scenario.title()}', color=colors.get(scenario, '#333333'), linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('CPU Usage (%)')
        plt.title('CPU Usage Scalability')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.ylim(0, 100)
        
        # 4. 메모리 사용률 vs 클라이언트 수
        plt.subplot(2, 3, 4)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['memory_usage_percent'], 
                    marker='d', label=f'{scenario.title()}', color=colors.get(scenario, '#333333'), linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Memory Usage (%)')
        plt.title('Memory Usage Scalability')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.ylim(0, 100)
        
        # 5. 연결 성공률 vs 클라이언트 수
        plt.subplot(2, 3, 5)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['connection_success_rate'], 
                    marker='v', label=f'{scenario.title()}', color=colors.get(scenario, '#333333'), linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Connection Success Rate (%)')
        plt.title('Connection Reliability')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.ylim(0, 105)
        
        # 6. 종합 성능 점수 vs 클라이언트 수
        plt.subplot(2, 3, 6)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['overall_score'], 
                    marker='*', markersize=8, label=f'{scenario.title()}', color=colors.get(scenario, '#333333'), linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Overall Score')
        plt.title('Overall Performance Score')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.ylim(0, 100)
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/scalability_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"확장성 분석 차트 저장: {output_dir}/scalability_analysis.png")
    
    def create_performance_heatmap(self, df, output_dir='performance_charts'):
        """성능 히트맵 생성"""
        os.makedirs(output_dir, exist_ok=True)
        
        # 피벗 테이블 생성 (시나리오 x 클라이언트 수)
        pivot_data = df.pivot_table(
            values=['packets_per_sec', 'p95_latency_ms', 'cpu_usage_percent', 'overall_score'],
            index='scenario',
            columns='client_count',
            aggfunc='mean'
        )
        
        fig, axes = plt.subplots(2, 2, figsize=(20, 12))
        
        # 1. 처리량 히트맵
        sns.heatmap(pivot_data['packets_per_sec'], annot=True, fmt='.0f', 
                   cmap='Blues', ax=axes[0,0], cbar_kws={'label': 'Packets/sec'})
        axes[0,0].set_title('Throughput Heatmap (Packets per Second)')
        axes[0,0].set_xlabel('Client Count')
        axes[0,0].set_ylabel('Scenario')
        
        # 2. 지연시간 히트맵
        sns.heatmap(pivot_data['p95_latency_ms'], annot=True, fmt='.1f', 
                   cmap='Reds', ax=axes[0,1], cbar_kws={'label': 'ms'})
        axes[0,1].set_title('P95 Latency Heatmap (ms)')
        axes[0,1].set_xlabel('Client Count')
        axes[0,1].set_ylabel('Scenario')
        
        # 3. CPU 사용률 히트맵
        sns.heatmap(pivot_data['cpu_usage_percent'], annot=True, fmt='.1f', 
                   cmap='Oranges', ax=axes[1,0], cbar_kws={'label': '%'})
        axes[1,0].set_title('CPU Usage Heatmap (%)')
        axes[1,0].set_xlabel('Client Count')
        axes[1,0].set_ylabel('Scenario')
        
        # 4. 종합 점수 히트맵
        sns.heatmap(pivot_data['overall_score'], annot=True, fmt='.0f', 
                   cmap='Greens', ax=axes[1,1], cbar_kws={'label': 'Score'})
        axes[1,1].set_title('Overall Performance Score Heatmap')
        axes[1,1].set_xlabel('Client Count')
        axes[1,1].set_ylabel('Scenario')
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/performance_heatmap.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"성능 히트맵 저장: {output_dir}/performance_heatmap.png")
    
    def create_bottleneck_analysis(self, df, output_dir='performance_charts'):
        """병목 지점 분석 차트 생성"""
        os.makedirs(output_dir, exist_ok=True)
        
        plt.figure(figsize=(18, 12))
        
        # 1. 처리량 효율성 분석
        plt.subplot(2, 3, 1)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            # 클라이언트당 처리량 계산
            throughput_per_client = scenario_data['packets_per_sec'] / scenario_data['client_count']
            plt.plot(scenario_data['client_count'], throughput_per_client, 
                    marker='o', label=f'{scenario.title()}', linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Packets per Second per Client')
        plt.title('Throughput Efficiency\n(Lower = Bottleneck)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # 2. 지연시간 증가율 분석
        plt.subplot(2, 3, 2)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            if len(scenario_data) > 1:
                latency_increase = scenario_data['p95_latency_ms'].pct_change() * 100
                plt.plot(scenario_data['client_count'].iloc[1:], latency_increase.iloc[1:], 
                        marker='s', label=f'{scenario.title()}', linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('P95 Latency Increase (%)')
        plt.title('Latency Degradation Rate\n(Higher = Bottleneck)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # 3. 리소스 사용률 vs 성능
        plt.subplot(2, 3, 3)
        colors = plt.cm.viridis(np.linspace(0, 1, len(df)))
        scatter = plt.scatter(df['cpu_usage_percent'], df['packets_per_sec'], 
                            c=df['client_count'], cmap='viridis', alpha=0.7, s=100)
        plt.xlabel('CPU Usage (%)')
        plt.ylabel('Packets per Second')
        plt.title('Resource Usage vs Performance')
        plt.colorbar(scatter, label='Client Count')
        plt.grid(True, alpha=0.3)
        
        # 4. 오류율 vs 부하
        plt.subplot(2, 3, 4)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            plt.plot(scenario_data['client_count'], scenario_data['error_rate_percent'], 
                    marker='^', label=f'{scenario.title()}', linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Error Rate (%)')
        plt.title('Error Rate vs Load\n(Higher = System Stress)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.yscale('log')
        
        # 5. 메모리 vs 지연시간 상관관계
        plt.subplot(2, 3, 5)
        scatter = plt.scatter(df['memory_usage_percent'], df['p95_latency_ms'], 
                            c=df['client_count'], cmap='plasma', alpha=0.7, s=100)
        plt.xlabel('Memory Usage (%)')
        plt.ylabel('P95 Latency (ms)')
        plt.title('Memory vs Latency Correlation')
        plt.colorbar(scatter, label='Client Count')
        plt.grid(True, alpha=0.3)
        plt.yscale('log')
        
        # 6. 성능 저하 임계점 분석
        plt.subplot(2, 3, 6)
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario].sort_values('client_count')
            # 성능 점수의 감소율
            if len(scenario_data) > 1:
                score_change = scenario_data['overall_score'].diff()
                plt.plot(scenario_data['client_count'].iloc[1:], score_change.iloc[1:], 
                        marker='*', markersize=8, label=f'{scenario.title()}', linewidth=2)
        
        plt.xlabel('Client Count')
        plt.ylabel('Performance Score Change')
        plt.title('Performance Degradation Points\n(Negative = Degradation)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.axhline(y=0, color='red', linestyle='--', alpha=0.5)
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/bottleneck_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"병목 분석 차트 저장: {output_dir}/bottleneck_analysis.png")
    
    def generate_performance_report(self, df, output_dir='performance_charts'):
        """성능 분석 리포트 생성"""
        os.makedirs(output_dir, exist_ok=True)
        
        report_file = f"{output_dir}/performance_analysis_report.md"
        
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write("# CppMMO 서버 성능 분석 리포트\n\n")
            f.write(f"생성 시간: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            # 전체 요약
            f.write("## 전체 요약\n\n")
            f.write(f"- 분석된 테스트 수: {len(df)}\n")
            f.write(f"- 테스트된 시나리오: {', '.join(df['scenario'].unique())}\n")
            f.write(f"- 클라이언트 수 범위: {df['client_count'].min()} ~ {df['client_count'].max()}\n\n")
            
            # 시나리오별 분석
            for scenario in df['scenario'].unique():
                scenario_data = df[df['scenario'] == scenario]
                f.write(f"## {scenario.title()} 시나리오 분석\n\n")
                
                # 최고 성능 지점
                best_performance = scenario_data.loc[scenario_data['overall_score'].idxmax()]
                f.write(f"### 최고 성능 지점\n")
                f.write(f"- 클라이언트 수: {best_performance['client_count']}\n")
                f.write(f"- 종합 점수: {best_performance['overall_score']:.1f}/100\n")
                f.write(f"- 처리량: {best_performance['packets_per_sec']:.1f} packets/sec\n")
                f.write(f"- P95 지연시간: {best_performance['p95_latency_ms']:.2f} ms\n\n")
                
                # 성능 저하 시작점 찾기
                sorted_data = scenario_data.sort_values('client_count')
                if len(sorted_data) > 1:
                    score_diff = sorted_data['overall_score'].diff()
                    degradation_start = sorted_data[score_diff < -5]  # 5점 이상 감소
                    
                    if not degradation_start.empty:
                        first_degradation = degradation_start.iloc[0]
                        f.write(f"### 성능 저하 시작점\n")
                        f.write(f"- 클라이언트 수: {first_degradation['client_count']}\n")
                        f.write(f"- CPU 사용률: {first_degradation['cpu_usage_percent']:.1f}%\n")
                        f.write(f"- 메모리 사용률: {first_degradation['memory_usage_percent']:.1f}%\n\n")
                
                # 권장 최대 클라이언트 수
                stable_performance = scenario_data[
                    (scenario_data['error_rate_percent'] < 1.0) & 
                    (scenario_data['p95_latency_ms'] < 100.0) &
                    (scenario_data['connection_success_rate'] > 95.0)
                ]
                
                if not stable_performance.empty:
                    max_recommended = stable_performance['client_count'].max()
                    f.write(f"### 권장 최대 클라이언트 수\n")
                    f.write(f"- 안정적 운영 가능: {max_recommended}명\n")
                    f.write(f"- 조건: 오류율 < 1%, P95 지연시간 < 100ms, 연결 성공률 > 95%\n\n")
                
                f.write("---\n\n")
            
            # 개선 권장사항
            f.write("## 개선 권장사항\n\n")
            
            # CPU 병목 체크
            high_cpu = df[df['cpu_usage_percent'] > 80]
            if not high_cpu.empty:
                f.write("### CPU 최적화 필요\n")
                f.write(f"- {len(high_cpu)}개 테스트에서 CPU 사용률 > 80%\n")
                f.write("- 권장사항: 프로파일링을 통한 핫스팟 최적화, 멀티스레딩 개선\n\n")
            
            # 메모리 병목 체크
            high_memory = df[df['memory_usage_percent'] > 80]
            if not high_memory.empty:
                f.write("### 메모리 최적화 필요\n")
                f.write(f"- {len(high_memory)}개 테스트에서 메모리 사용률 > 80%\n")
                f.write("- 권장사항: 메모리 풀 도입, 불필요한 할당 최소화\n\n")
            
            # 지연시간 개선
            high_latency = df[df['p95_latency_ms'] > 100]
            if not high_latency.empty:
                f.write("### 지연시간 개선 필요\n")
                f.write(f"- {len(high_latency)}개 테스트에서 P95 지연시간 > 100ms\n")
                f.write("- 권장사항: 네트워크 버퍼 튜닝, 알고리즘 최적화, 캐싱 도입\n\n")
            
            # 안정성 개선
            low_reliability = df[df['connection_success_rate'] < 95]
            if not low_reliability.empty:
                f.write("### 연결 안정성 개선 필요\n")
                f.write(f"- {len(low_reliability)}개 테스트에서 연결 성공률 < 95%\n")
                f.write("- 권장사항: 타임아웃 조정, 재연결 로직 강화, 에러 핸들링 개선\n\n")
        
        print(f"성능 분석 리포트 저장: {report_file}")
    
    def run_analysis(self):
        """전체 분석 실행"""
        print("CppMMO 서버 성능 분석 시작...")
        
        # 데이터 로드
        self.load_performance_data()
        self.load_csv_data()
        
        if not self.performance_data:
            print("분석할 성능 데이터가 없습니다.")
            print("먼저 성능 테스트를 실행하여 데이터를 생성하세요:")
            print("python comprehensive_performance_test.py --scenario basic")
            return
        
        # 메트릭 추출
        df = self.extract_performance_metrics()
        
        if df.empty:
            print("메트릭 추출에 실패했습니다.")
            return
        
        print(f"추출된 데이터: {len(df)}개 테스트 결과")
        print("시나리오별 클라이언트 수:")
        for scenario in df['scenario'].unique():
            scenario_data = df[df['scenario'] == scenario]
            client_counts = sorted(scenario_data['client_count'].unique())
            print(f"  {scenario}: {client_counts}")
        
        # 차트 생성
        output_dir = f"performance_analysis_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        
        print(f"\n차트 생성 중... (저장 위치: {output_dir})")
        self.create_scalability_charts(df, output_dir)
        self.create_performance_heatmap(df, output_dir)
        self.create_bottleneck_analysis(df, output_dir)
        
        # 리포트 생성
        print("성능 분석 리포트 생성 중...")
        self.generate_performance_report(df, output_dir)
        
        print(f"\n✅ 분석 완료!")
        print(f"📁 결과 디렉토리: {output_dir}")
        print(f"📊 생성된 차트:")
        print(f"   - scalability_analysis.png")
        print(f"   - performance_heatmap.png") 
        print(f"   - bottleneck_analysis.png")
        print(f"📄 분석 리포트: performance_analysis_report.md")
        
        return df, output_dir

def main():
    parser = argparse.ArgumentParser(description='CppMMO 서버 성능 분석')
    parser.add_argument('--data-dir', '-d', 
                        default='.',
                        help='성능 데이터 파일이 있는 디렉토리')
    parser.add_argument('--generate-test-data', 
                        action='store_true',
                        help='테스트 데이터 생성 스크립트 출력')
    
    args = parser.parse_args()
    
    if args.generate_test_data:
        print("다양한 클라이언트 수로 테스트를 실행하려면 다음 명령어들을 순차적으로 실행하세요:\n")
        
        client_counts = [10, 25, 50, 100, 200, 300, 500]
        scenarios = ['basic', 'stress']
        
        for scenario in scenarios:
            print(f"# {scenario.title()} 시나리오")
            for count in client_counts:
                # 시나리오별로 설정 조정
                duration = 120 if scenario == 'basic' else 180
                print(f"# {count}명 클라이언트 테스트")
                print(f"python comprehensive_performance_test.py --scenario {scenario}")
                print("sleep 30  # 서버 안정화 대기")
                print()
        
        print("모든 테스트가 완료된 후:")
        print("python performance_analysis.py")
        return
    
    analyzer = PerformanceAnalyzer(args.data_dir)
    analyzer.run_analysis()

if __name__ == "__main__":
    main()