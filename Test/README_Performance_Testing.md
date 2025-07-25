# CppMMO 서버 성능 테스트 가이드

CppMMO 서버의 성능을 체계적으로 측정하고 분석하는 도구 모음입니다. pandas와 matplotlib을 활용하여 인원수별 성능 차이를 시각화합니다.

## 📋 목차

1. [개요](#-개요)
2. [설치 및 준비](#️-설치-및-준비)
3. [기본 사용법](#-기본-사용법)
4. [고급 사용법](#-고급-사용법)
5. [결과 분석](#-결과-분석)
6. [문제 해결](#-문제-해결)

## 🎯 개요

### 주요 기능
- **확장성 테스트**: 다양한 클라이언트 수(10~500명)로 서버 성능 측정
- **성능 지표 측정**: 처리량, 지연시간, CPU/메모리 사용률, 안정성
- **시각화 분석**: pandas + matplotlib 기반 차트 및 히트맵 생성
- **자동화**: 인원수별 테스트 자동 실행 및 결과 수집

### 측정 지표
- **처리량 (Throughput)**: 패킷/초, 대역폭 사용량
- **지연시간 (Latency)**: 평균, P95, P99 지연시간, RTT
- **시스템 리소스**: CPU 및 메모리 사용률
- **안정성**: 연결 성공률, 오류율

## 🛠️ 설치 및 준비

### 1. Python 패키지 설치
```bash
pip install pandas matplotlib seaborn numpy psutil
```

### 2. 서버 실행
CppMMO 서버가 localhost:8080에서 실행 중이어야 합니다.

### 3. FlatBuffers 프로토콜 파일 확인
`Test/Protocol/` 디렉토리에 FlatBuffers 생성 파일들이 있는지 확인하세요.

## 🚀 기본 사용법

### 1. 단일 성능 테스트
```bash
# 기본 시나리오 (50명, 2분)
python comprehensive_performance_test.py --scenario basic

# 스트레스 시나리오 (200명, 5분)
python comprehensive_performance_test.py --scenario stress

# 커스텀 클라이언트 수
python comprehensive_performance_test.py --scenario basic --clients 100

# 커스텀 지속 시간
python comprehensive_performance_test.py --scenario basic --duration 180
```

### 2. 확장성 테스트 (인원수별)
```bash
# 기본 확장성 테스트 (10, 25, 50, 100, 200, 300, 500명)
python run_scalability_tests.py

# 빠른 테스트 (10, 25, 50명만)
python run_scalability_tests.py --quick

# 커스텀 클라이언트 수 범위
python run_scalability_tests.py --clients 10 50 100 200

# 특정 시나리오만
python run_scalability_tests.py --scenarios basic
```

### 3. 결과 분석 및 시각화
```bash
# 모든 테스트 결과 분석
python performance_analysis.py

# 특정 디렉토리의 결과 분석
python performance_analysis.py --data-dir ./test_results
```

## 🔬 고급 사용법

### 1. 전체 테스트 스위트 실행
```bash
# 모든 시나리오 + 부하 테스트
python run_performance_tests.py

# 기본 시나리오만
python run_performance_tests.py --scenarios basic

# 부하 테스트 제외
python run_performance_tests.py --no-load-tests
```

### 2. 커스텀 테스트 시나리오
```python
# comprehensive_performance_test.py의 TestConfig.SCENARIOS에 추가
'custom': {
    'clients': 150,
    'duration': 240,  # 4분
    'movement_interval': 0.04,  # 25fps
    'chat_interval': 8.0
}
```

### 3. 병렬 테스트 실행
```bash
# 여러 터미널에서 동시 실행 (서로 다른 클라이언트 수)
# 터미널 1
python comprehensive_performance_test.py --scenario basic --clients 50

# 터미널 2 (30초 후)
python comprehensive_performance_test.py --scenario basic --clients 100
```

## 📊 결과 분석

### 생성되는 파일들
```
📁 Test/
├── performance_results_YYYYMMDD_HHMMSS.json  # 종합 성능 결과
├── performance_test_YYYYMMDD_HHMMSS.csv      # 시계열 데이터
├── scalability_test_summary_YYYYMMDD_HHMMSS.json  # 확장성 테스트 요약
└── performance_analysis_YYYYMMDD_HHMMSS/     # 분석 결과 디렉토리
    ├── scalability_analysis.png              # 확장성 차트
    ├── performance_heatmap.png               # 성능 히트맵
    ├── bottleneck_analysis.png               # 병목 분석 차트
    └── performance_analysis_report.md        # 분석 리포트
```

### 차트 해석

#### 1. Scalability Analysis (scalability_analysis.png)
- **처리량 vs 클라이언트 수**: 선형 증가하다가 포화점에서 평평해짐
- **지연시간 vs 클라이언트 수**: 일정 수준까지는 낮다가 급격히 증가
- **CPU/메모리 사용률**: 리소스 병목 지점 확인
- **연결 성공률**: 안정성 한계점 파악
- **종합 성능 점수**: 전반적인 성능 트렌드

#### 2. Performance Heatmap (performance_heatmap.png)
- 시나리오별 × 클라이언트 수별 성능 매트릭스
- 색상이 진할수록 높은 값 (처리량은 좋음, 지연시간은 나쁨)
- 최적 운영 구간과 성능 저하 구간 시각적 확인

#### 3. Bottleneck Analysis (bottleneck_analysis.png)
- **클라이언트당 처리량**: 효율성 감소 지점
- **지연시간 증가율**: 성능 저하 시작점
- **리소스 vs 성능**: 병목 원인 분석
- **오류율**: 시스템 한계점

### 성능 등급
- **S급 (90-100점)**: 우수한 성능, 프로덕션 준비 완료
- **A급 (80-89점)**: 좋은 성능, 소규모 최적화 권장
- **B급 (70-79점)**: 보통 성능, 성능 개선 필요
- **C급 (60-69점)**: 미흡한 성능, 상당한 최적화 필요
- **D급 (60점 미만)**: 부족한 성능, 아키텍처 재검토 필요

## 📈 실행 예시

### 전체 워크플로우
```bash
# 1. 서버 실행 확인
netstat -an | findstr :8080  # Windows
netstat -an | grep :8080     # Linux/Mac

# 2. 확장성 테스트 실행 (약 30-60분 소요)
python run_scalability_tests.py --scenarios basic stress

# 3. 결과 분석 및 시각화
python performance_analysis.py

# 4. 결과 확인
# - performance_analysis_YYYYMMDD_HHMMSS/ 디렉토리 확인
# - .png 차트 파일들 확인
# - performance_analysis_report.md 리포트 확인
```

### 빠른 테스트 (개발 중)
```bash
# 개발 중 빠른 성능 확인 (약 5-10분)
python run_scalability_tests.py --quick
python performance_analysis.py
```

### 특정 클라이언트 수 집중 테스트
```bash
# 예상 운영 규모 주변 집중 테스트
python run_scalability_tests.py --clients 80 100 120 150 180 200
```

## 🐛 문제 해결

### 일반적인 문제들

#### 1. "Could not import FlatBuffers modules" 오류
```bash
# FlatBuffers 프로토콜 파일 재생성 (프로젝트 루트에서 실행)
cd <프로젝트_루트_경로>
# Python 테스트용 프로토콜 파일 생성
flatc --python --gen-object-api -o Test/Protocol src/Common/protocol.fbs

# 또는 환경 변수 사용
# export CPPMMO_ROOT=/path/to/project
# cd $CPPMMO_ROOT
# flatc --python --gen-object-api -o Test/Protocol src/Common/protocol.fbs
```

#### 2. "서버에 연결할 수 없습니다" 오류
- CppMMO 서버가 localhost:8080에서 실행 중인지 확인
- 방화벽 설정 확인
- 다른 프로세스가 포트를 사용하고 있는지 확인

#### 3. 메모리 부족 오류
- 클라이언트 수를 줄이거나 테스트 시간을 단축
- 테스트 간 대기 시간을 늘려서 메모리 정리 시간 확보

#### 4. 테스트 결과가 생성되지 않음
```bash
# 권한 확인
ls -la *.json *.csv  # Linux/Mac
dir *.json *.csv     # Windows

# 수동으로 단일 테스트 실행
python comprehensive_performance_test.py --scenario basic --clients 10
```

#### 5. 차트가 표시되지 않음 (matplotlib 관련)
```bash
# 필요한 패키지 재설치
pip install --upgrade matplotlib seaborn

# 백엔드 문제인 경우
pip install PyQt5  # 또는 tkinter
```

### 성능 최적화 권장사항

#### 서버 설정
- 충분한 메모리 할당 (8GB 이상 권장)
- SSD 스토리지 사용
- 네트워크 대역폭 확보

#### 테스트 환경
- 테스트 중 다른 네트워크 집약적 작업 중단
- 안정적인 전원 공급
- 충분한 디스크 공간 (1GB 이상)

## 📚 추가 정보

### 성능 데이터 활용
- CI/CD 파이프라인에 통합하여 성능 회귀 감지
- 정기적인 성능 모니터링으로 트렌드 분석
- 서버 스케일링 계획 수립에 활용

### 커스터마이징
- `TestConfig.SCENARIOS`에 새 시나리오 추가
- `PerformanceAnalyzer`에 새 메트릭 추가
- 차트 스타일 및 색상 커스터마이징

### 문의 및 기여
성능 테스트 도구 개선사항이나 버그 리포트는 프로젝트 이슈에 등록해주세요.

---

**마지막 업데이트**: 2024-07-25  
**호환 버전**: CppMMO Server v1.0+, Python 3.7+