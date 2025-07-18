# CppMMO: Modern C++ MMORPG Server

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/CMake-3.15+-green)
![Docker](https://img.shields.io/badge/Docker-Enabled-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)
![Status](https://img.shields.io/badge/Status-Production%20Ready-success)

**완전 구현된 Server Authority 기반 실시간 멀티플레이어 MMO 서버**

C++20, Boost.Asio, FlatBuffers를 기반으로 제작된 고성능 현대적 MMORPG 서버입니다. 60 TPS 게임 루프, QuadTree 공간 최적화, 실시간 플레이어 동기화를 구현했습니다.

## 🎉 **완성된 기능들**

### ✅ **핵심 서버 시스템**
- **60 TPS 게임 루프**: 매 16.67ms마다 안정적인 월드 업데이트
- **Server Authority**: 서버 권한 기반 위치 검증 및 상태 관리
- **QuadTree AOI**: 100 반경 Area of Interest 공간 최적화
- **실시간 동기화**: 60Hz 월드 스냅샷 브로드캐스트
- **완전한 멀티플레이어**: 플레이어 입장/퇴장/이동 동기화

### ✅ **게임 시스템**
- **플레이어 관리**: 스폰, 이동, 연결 해제 처리
- **존 시스템**: 동적 플레이어 입장/퇴장 관리
- **채팅 시스템**: Redis 기반 50 유닛 범위 채팅
- **입력 시스템**: WASD 이동, 시퀀스 번호 검증
- **재연결 시스템**: 5분 타임아웃 기반 재연결 지원

### ✅ **네트워크 & 인증**
- **FlatBuffers 프로토콜**: 효율적인 바이너리 직렬화
- **세션 관리**: 안전한 연결 생성/해제 감지
- **인증 서버 연동**: ASP.NET Core 기반 토큰 검증
- **패킷 처리**: 멀티스레드 안전한 패킷 큐 시스템

## 🏗️ **아키텍처 개요**

### **서비스 구성**
```
┌─────────────────┐    ┌─────────────────┐
│   Unity Client  │────│  CppMMO Server  │
│   (개발 예정)    │    │   Port: 8080    │
└─────────────────┘    └─────────────────┘
                               │
                    ┌──────────┼──────────┐
                    │          │          │
            ┌───────────┐ ┌─────────┐ ┌─────────┐
            │AuthServer │ │  Redis  │ │  MySQL  │
            │Port: 5278 │ │Port:6379│ │Port:3306│
            └───────────┘ └─────────┘ └─────────┘
```

### **게임 서버 내부 구조**
```
┌─────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Clients   │────▶│   TcpServer     │────▶│  SessionManager │
└─────────────┘     │  (Network I/O)  │     │  (Connection)   │
                    └─────────────────┘     └─────────────────┘
                              │                        │
                              ▼                        ▼
                    ┌─────────────────┐     ┌─────────────────┐
                    │  PacketManager  │────▶│   JobProcessor  │
                    │  (Validation)   │     │ (Packet Parsing)│
                    └─────────────────┘     └─────────────────┘
                                                      │
                                                      ▼
                                            ┌─────────────────┐
                                            │  GameManager    │
                                            │  (60 TPS Loop)  │
                                            │  • World Update │
                                            │  • QuadTree AOI │
                                            │  • Player Sync  │
                                            └─────────────────┘
```

## 🎮 **게임 플로우**

### **완전 구현된 플레이어 라이프사이클**
1. **연결**: TCP 연결 → 세션 생성
2. **인증**: C_Login → AuthServer 검증 → S_LoginSuccess
3. **입장**: C_EnterZone → 스폰 위치 생성 → S_ZoneEntered
4. **게임**: C_PlayerInput → 서버 시뮬레이션 → S_WorldSnapshot
5. **퇴장**: 연결 해제 → S_PlayerLeft 브로드캐스트

### **실시간 동기화 시스템**
- **입력 처리**: 시퀀스 번호 기반 중복 방지
- **월드 업데이트**: 60 TPS 서버 시뮬레이션
- **AOI 시스템**: 100 반경 내 플레이어만 동기화
- **스냅샷 전송**: 60Hz 월드 상태 브로드캐스트

## 🚀 **빠른 시작**

### **1. Docker를 이용한 실행 (권장)**
```bash
# 저장소 클론
git clone <your-repo-url>
cd CppMMO

# 모든 서비스 실행
docker-compose up -d

# 서비스 상태 확인
docker-compose ps
```

### **2. 서비스 상태 확인**
```bash
# 서버 로그 확인
docker-compose logs -f cppmmo_server

# 개별 서비스 로그
docker-compose logs authserver
docker-compose logs redis
docker-compose logs mysql
```

## 📡 **네트워크 프로토콜**

### **FlatBuffers 기반 바이너리 통신**
모든 패킷은 `UnifiedPacket` 형태로 전송됩니다:

```fbs
table UnifiedPacket {
    id: PacketId;
    data: Packet;
}
```

### **핵심 패킷 타입**

#### **클라이언트 → 서버**
- `C_Login (1)`: 로그인 요청
- `C_PlayerInput (10)`: WASD 입력 (비트 플래그)
- `C_EnterZone (20)`: 존 입장 요청
- `C_Chat (4)`: 채팅 메시지

#### **서버 → 클라이언트**
- `S_LoginSuccess (2)`: 로그인 성공 + 플레이어 정보
- `S_ZoneEntered (21)`: 존 입장 성공 + 근처 플레이어들
- `S_WorldSnapshot (11)`: 월드 상태 스냅샷 (60Hz)
- `S_PlayerJoined (22)`: 플레이어 입장 알림
- `S_PlayerLeft (23)`: 플레이어 퇴장 알림

### **입력 시스템**
WASD 입력은 비트 플래그로 처리됩니다:
```cpp
W = 1    // 0001
S = 2    // 0010  
A = 4    // 0100
D = 8    // 1000
```

## ⚙️ **서버 설정**

### **게임 설정** (`config/game_config.json`)
```json
{
    "map": {
        "width": 200.0,
        "height": 200.0
    },
    "gameplay": {
        "chat_range": 50.0,
        "aoi_range": 100.0,
        "move_speed": 5.0,
        "tick_rate": 60
    },
    "network": {
        "snapshot_rate": 60,
        "reconnect_timeout_minutes": 5
    }
}
```

### **주요 성능 지표**
- **게임 틱**: 60 TPS (16.67ms)
- **스냅샷 전송**: 60Hz (16.67ms)
- **맵 크기**: 200x200 유닛
- **이동 속도**: 5.0 유닛/초
- **AOI 범위**: 100 유닛
- **채팅 범위**: 50 유닛

## 🎯 **클라이언트 개발 가이드**

### **📚 제공 문서**
- **[SERVER_DOCUMENTATION.md](SERVER_DOCUMENTATION.md)**: 서버 전체 가이드
- **[CLIENT_EXAMPLES.md](CLIENT_EXAMPLES.md)**: Unity 클라이언트 구현 예제
- **[PROTOCOL_REFERENCE.md](PROTOCOL_REFERENCE.md)**: FlatBuffers 프로토콜 완전 가이드

### **Unity 클라이언트 개발 순서**
1. **FlatBuffers Unity 패키지 설치**
2. **네트워크 클라이언트 구현** (TCP 연결)
3. **인증 시스템 구현** (C_Login)
4. **플레이어 입력 시스템** (C_PlayerInput)
5. **월드 동기화 시스템** (S_WorldSnapshot)
6. **멀티플레이어 구현** (플레이어 입장/퇴장)

### **연결 테스트**
```bash
# 서버 연결 테스트
telnet localhost 8080

# 서비스 포트 확인
docker-compose ps
```

## 🏗️ **프로젝트 구조**

```
CppMMO/
├── src/                           # C++ 게임 서버 소스코드
│   ├── Common/                   # 공통 프로토콜 정의
│   │   ├── protocol.fbs          # FlatBuffers 스키마
│   │   └── protocol_generated.h  # 생성된 헤더
│   ├── Network/                  # 네트워크 시스템
│   │   ├── TcpServer.*           # TCP 서버
│   │   ├── Session.*             # 세션 관리
│   │   ├── SessionManager.*      # 세션 매니저
│   │   └── PacketManager.*       # 패킷 처리
│   ├── Game/                     # 게임 로직
│   │   ├── Managers/             # 게임 매니저들
│   │   │   ├── GameManager.*     # 핵심 게임 로직 (60 TPS)
│   │   │   └── ChatManager.*     # 채팅 시스템
│   │   ├── Models/               # 게임 모델
│   │   │   ├── Player.*          # 플레이어 모델
│   │   │   └── World.*           # 월드 모델
│   │   ├── Spatial/              # 공간 최적화
│   │   │   └── QuadTree.*        # QuadTree AOI
│   │   ├── PacketHandlers/       # 패킷 핸들러
│   │   ├── Services/             # 외부 서비스 연동
│   │   └── GameCommand.h         # 게임 명령 정의
│   └── Utils/                    # 유틸리티
│       ├── Logger.*              # 로깅 시스템
│       ├── JobProcessor.*        # 작업 처리
│       └── JobQueue.*            # 작업 큐
├── auth/                         # .NET 인증 서버
├── config/                       # 설정 파일
│   └── game_config.json          # 게임 설정
├── docker-compose.yml            # Docker 컨테이너 설정
├── Dockerfile                    # 게임 서버 Docker 이미지
├── CMakeLists.txt               # CMake 빌드 설정
├── SERVER_DOCUMENTATION.md      # 서버 가이드 문서
├── CLIENT_EXAMPLES.md           # Unity 클라이언트 예제
├── PROTOCOL_REFERENCE.md        # 프로토콜 참조 문서
└── README.md                    # 이 파일
```

## 🧪 **테스트 및 검증**

### **서버 로그 확인**
```bash
# 실시간 로그 모니터링
docker-compose logs -f cppmmo_server

# 특정 플레이어 로그 필터링
docker-compose logs cppmmo_server | grep "Player 12345"

# 게임 플로우 로그 확인
docker-compose logs cppmmo_server | grep -E "(Login|Player|Zone|Input)"
```

### **성능 모니터링**
- **동시 접속자**: 테스트 완료 (다중 세션 지원)
- **메모리 사용량**: 안정적인 메모리 관리
- **CPU 사용률**: 60 TPS 안정적 유지
- **네트워크 처리량**: 실시간 패킷 처리

## 📊 **구현 현황**

### ✅ **완료된 시스템**
- **네트워크 시스템**: TCP 서버, 세션 관리, 패킷 처리
- **인증 시스템**: AuthServer 연동, 토큰 검증
- **게임 로직**: 60 TPS 게임 루프, 월드 업데이트
- **플레이어 시스템**: 스폰, 이동, 상태 관리
- **공간 시스템**: QuadTree AOI, 거리 기반 최적화
- **동기화 시스템**: 실시간 월드 스냅샷, 플레이어 동기화
- **채팅 시스템**: Redis 기반 Pub/Sub 채팅

## 🛠️ **기술 스택**

### **서버 (C++)**
- **언어**: C++20
- **네트워크**: Boost.Asio
- **직렬화**: FlatBuffers
- **로깅**: spdlog
- **데이터베이스**: MySQL 8.0
- **캐시**: Redis
- **빌드**: CMake 3.15+
- **컨테이너**: Docker

### **인증 서버 (.NET)**
- **프레임워크**: ASP.NET Core 8
- **API**: RESTful HTTP
- **인증**: Session Ticket

### **클라이언트 (Unity)**
- **엔진**: Unity 2022.3 LTS
- **직렬화**: FlatBuffers Unity Package
- **네트워크**: System.Net.Sockets

## 🔧 **로컬 개발 환경**

### **요구사항**
- C++20 지원 컴파일러 (GCC 13+, MSVC v143+)
- CMake 3.15+
- Docker & Docker Compose
- .NET 8 SDK (인증 서버)

### **빌드 과정**
```bash
# 의존성 설치 (Docker 방식)
docker-compose build

# 로컬 빌드 (vcpkg 방식)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 실행
./build/bin/CppMMO_Deployment
```

## 📄 **라이센스**

이 프로젝트는 MIT 라이센스 하에 배포됩니다.

## 🤝 **기여하기**

1. Fork this repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request