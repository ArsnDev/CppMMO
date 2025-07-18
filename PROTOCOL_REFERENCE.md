# CppMMO Protocol Reference

## 📋 FlatBuffers 프로토콜 정의

### 🌟 **프로토콜 개요**
- **직렬화 형식**: FlatBuffers
- **네임스페이스**: `CppMMO.Protocol`
- **루트 타입**: `UnifiedPacket`
- **패킷 방식**: 타입 안전한 Union 기반

---

## 🏗️ **기본 데이터 구조**

### **Vec3 (3D 벡터)**
```fbs
table Vec3 {
  x:float;
  y:float;
  z:float;  // 2D 게임에서는 z=0 사용
}
```

### **GameTick (게임 틱 정보)**
```fbs
table GameTick {
  tick_number:ulong;
  server_time:ulong;
}
```

### **PlayerInfo (플레이어 정보)**
```fbs
table PlayerInfo {
  player_id:ulong;
  name:string;
  position:Vec3;
  hp:int;
  max_hp:int;
  mp:int;
  max_mp:int;
}
```

### **PlayerState (플레이어 상태)**
```fbs
table PlayerState {
  player_id:ulong;
  position:Vec3;
  velocity:Vec3;
  rotation:float;
  hp:int;
  mp:int;
  last_input_sequence:uint;
}
```

---

## 🎮 **게임 이벤트 시스템**

### **EventType (이벤트 타입)**
```fbs
enum EventType : ubyte {
  NONE = 0,
  PLAYER_DAMAGE = 1,
  PLAYER_HEAL = 2,
  PLAYER_DEATH = 3,
  PLAYER_RESPAWN = 4,
}
```

### **GameEvent (게임 이벤트)**
```fbs
table GameEvent {
  event_type:EventType;
  source_player_id:ulong;
  target_player_id:ulong;
  position:Vec3;
  value:int;  // 데미지, 힐량 등
}
```

---

## 📦 **패킷 ID 정의**

### **PacketId 열거형**
```fbs
enum PacketId : ushort {
  NONE = 0,
  
  // === 인증 & 채팅 ===
  C_Login = 1,
  S_LoginSuccess = 2,
  S_LoginFailure = 3,
  C_Chat = 4,
  S_Chat = 5,
  
  // === 게임 플레이 ===
  C_PlayerInput = 10,
  S_WorldSnapshot = 11,
  S_StateCorrection = 12,
  S_GameTick = 13,
  
  // === 존 시스템 ===
  C_EnterZone = 20,
  S_ZoneEntered = 21,
  S_PlayerJoined = 22,
  S_PlayerLeft = 23,
}
```

---

## 🔐 **인증 & 채팅 패킷**

### **C_Login (클라이언트 → 서버)**
```fbs
table C_Login {
  session_ticket:string;
  command_id:long;
}
```
- **용도**: 로그인 요청
- **필수 필드**: `session_ticket`
- **응답**: `S_LoginSuccess` 또는 `S_LoginFailure`

### **S_LoginSuccess (서버 → 클라이언트)**
```fbs
table S_LoginSuccess {
  player_info:PlayerInfo;
  command_id:long;
}
```
- **용도**: 로그인 성공 응답
- **포함 정보**: 플레이어 ID, 이름, 위치, HP/MP

### **S_LoginFailure (서버 → 클라이언트)**
```fbs
table S_LoginFailure {
  error_code:int;
  error_message:string;
  command_id:long;
}
```
- **용도**: 로그인 실패 응답
- **오류 정보**: 에러 코드 및 메시지

### **C_Chat (클라이언트 → 서버)**
```fbs
table C_Chat {
  message:string;
  command_id:long;
}
```
- **용도**: 채팅 메시지 전송
- **범위**: 50 유닛 반경 내 플레이어들

### **S_Chat (서버 → 클라이언트)**
```fbs
table S_Chat {
  player_id:long;
  message:string;
  command_id:long;
}
```
- **용도**: 채팅 메시지 수신
- **포함 정보**: 발신자 ID, 메시지 내용

---

## 🎮 **게임 플레이 패킷**

### **C_PlayerInput (클라이언트 → 서버)**
```fbs
table C_PlayerInput {
  tick_number:ulong;
  client_time:ulong;
  input_flags:ubyte;        // WASD 비트 플래그
  mouse_position:Vec3;      // 마우스 위치
  sequence_number:uint;     // 입력 순서 보장
  command_id:long;
}
```

#### **입력 플래그 비트 마스크**
```
비트 0 (1): W (위쪽)
비트 1 (2): S (아래쪽)
비트 2 (4): A (왼쪽)
비트 3 (8): D (오른쪽)
비트 4 (16): Shift (달리기)
비트 5 (32): Space (점프)
```

#### **사용 예시**
```cpp
// C++
uint8_t inputFlags = 0;
if (pressedW) inputFlags |= 1;  // W
if (pressedD) inputFlags |= 8;  // D
// 결과: inputFlags = 9 (W + D = 대각선 이동)
```

```csharp
// C#
byte inputFlags = 0;
if (Input.GetKey(KeyCode.W)) inputFlags |= 1;
if (Input.GetKey(KeyCode.D)) inputFlags |= 8;
```

### **S_WorldSnapshot (서버 → 클라이언트)**
```fbs
table S_WorldSnapshot {
  tick_number:ulong;
  server_time:ulong;
  player_states:[PlayerState];  // AOI 내 플레이어들
  events:[GameEvent];           // 이번 틱의 이벤트들
}
```
- **전송 주기**: 20Hz (50ms마다)
- **범위**: AOI 100 유닛 반경 내
- **포함 정보**: 플레이어 위치, 속도, 상태

### **S_StateCorrection (서버 → 클라이언트)**
```fbs
table S_StateCorrection {
  tick_number:ulong;
  corrected_position:Vec3;
  corrected_velocity:Vec3;
  sequence_number:uint;     // 보정 시점의 입력 시퀀스
}
```
- **용도**: 클라이언트 예측 보정
- **발생 조건**: 서버-클라이언트 위치 차이 임계값 초과

### **S_GameTick (서버 → 클라이언트)**
```fbs
table S_GameTick {
  tick_info:GameTick;
}
```
- **용도**: 게임 틱 동기화
- **전송 조건**: 필요시 (현재 미사용)

---

## 🏘️ **존 시스템 패킷**

### **C_EnterZone (클라이언트 → 서버)**
```fbs
table C_EnterZone {
  zone_id:int;
  command_id:long;
}
```
- **용도**: 존 입장 요청
- **현재 지원**: Zone ID 1 (기본 존)

### **S_ZoneEntered (서버 → 클라이언트)**
```fbs
table S_ZoneEntered {
  zone_id:int;
  my_player:PlayerInfo;
  other_players:[PlayerInfo];
}
```
- **용도**: 존 입장 완료 응답
- **포함 정보**: 
  - 본인 플레이어 정보
  - 근처 플레이어들 목록 (AOI 범위 내)

### **S_PlayerJoined (서버 → 클라이언트)**
```fbs
table S_PlayerJoined {
  player_info:PlayerInfo;
}
```
- **용도**: 새 플레이어 입장 알림
- **수신 대상**: 기존 플레이어들

### **S_PlayerLeft (서버 → 클라이언트)**
```fbs
table S_PlayerLeft {
  player_id:ulong;
}
```
- **용도**: 플레이어 퇴장 알림
- **수신 대상**: 남은 플레이어들

---

## 📋 **패킷 Union 및 루트 타입**

### **Packet Union**
```fbs
union Packet {
  // Authentication & Chat
  C_Login,
  S_LoginSuccess,
  S_LoginFailure,
  C_Chat,
  S_Chat,
  
  // Server Authority Game
  C_PlayerInput,
  S_WorldSnapshot,
  S_StateCorrection,
  S_GameTick,
  
  // Zone System
  C_EnterZone,
  S_ZoneEntered,
  S_PlayerJoined,
  S_PlayerLeft,
}
```

### **UnifiedPacket (루트 타입)**
```fbs
table UnifiedPacket {
  id:PacketId;
  data:Packet;
}

root_type UnifiedPacket;
```

---

## 🔧 **클라이언트 구현 가이드**

### **1. 패킷 생성 예시 (C#)**
```csharp
// 로그인 패킷 생성
var builder = new FlatBufferBuilder(1024);
var sessionTicketOffset = builder.CreateString("my_session_ticket");
var loginPacket = C_Login.CreateC_Login(builder, sessionTicketOffset, 0);
var packet = UnifiedPacket.CreateUnifiedPacket(builder, 
    PacketId.C_Login, Packet.C_Login, loginPacket.Union());
builder.Finish(packet);

// 전송
byte[] data = builder.SizedByteArray();
networkStream.Write(data, 0, data.Length);
```

### **2. 패킷 파싱 예시 (C#)**
```csharp
// 패킷 수신 및 파싱
byte[] buffer = new byte[4096];
int bytesRead = networkStream.Read(buffer, 0, buffer.Length);

var byteBuffer = new ByteBuffer(buffer, 0, bytesRead);
var packet = UnifiedPacket.GetRootAsUnifiedPacket(byteBuffer);

switch (packet.Id)
{
    case PacketId.S_LoginSuccess:
        var loginSuccess = packet.Data<S_LoginSuccess>();
        Debug.Log($"Login successful! Player: {loginSuccess.PlayerInfo.Name}");
        break;
    
    case PacketId.S_WorldSnapshot:
        var snapshot = packet.Data<S_WorldSnapshot>();
        for (int i = 0; i < snapshot.PlayerStatesLength; i++)
        {
            var playerState = snapshot.PlayerStates(i);
            UpdatePlayer(playerState);
        }
        break;
}
```

### **3. 입력 처리 예시 (C#)**
```csharp
// WASD 입력을 비트 플래그로 변환
byte inputFlags = 0;
if (Input.GetKey(KeyCode.W)) inputFlags |= 1;   // W
if (Input.GetKey(KeyCode.S)) inputFlags |= 2;   // S
if (Input.GetKey(KeyCode.A)) inputFlags |= 4;   // A
if (Input.GetKey(KeyCode.D)) inputFlags |= 8;   // D

// 입력 패킷 생성
var builder = new FlatBufferBuilder(1024);
var inputPacket = C_PlayerInput.CreateC_PlayerInput(builder, 
    0, 0, inputFlags, mousePos, sequenceNumber++, 0);
var packet = UnifiedPacket.CreateUnifiedPacket(builder, 
    PacketId.C_PlayerInput, Packet.C_PlayerInput, inputPacket.Union());
```

---

## ⚡ **성능 최적화 팁**

### **1. FlatBuffers 최적화**
- 빌더 재사용: `FlatBufferBuilder`를 풀링
- 메모리 할당 최소화: 초기 버퍼 크기 적절히 설정
- 직렬화 비용 최소화: 불필요한 필드 제거

### **2. 네트워크 최적화**
- 입력 패킷 배칭: 여러 입력을 묶어서 전송
- 압축 고려: 반복적인 데이터 압축
- 우선순위 큐: 중요한 패킷 우선 처리

### **3. 게임 로직 최적화**
- 예측 시스템: 클라이언트 예측으로 지연 시간 감소
- 보간 시스템: 부드러운 움직임을 위한 위치 보간
- AOI 최적화: 필요한 플레이어만 업데이트

---

## 🐛 **디버깅 도구**

### **1. 패킷 로그**
```csharp
// 패킷 송신 로그
Debug.Log($"[SEND] {packetId} - Size: {data.Length} bytes");

// 패킷 수신 로그
Debug.Log($"[RECV] {packet.Id} - Players: {snapshot.PlayerStatesLength}");
```

### **2. 서버 로그 모니터링**
```bash
# 실시간 서버 로그
docker-compose logs -f cppmmo_server

# 특정 패킷 필터링
docker-compose logs cppmmo_server | grep "PlayerInput"
```

### **3. 네트워크 분석**
```bash
# 패킷 캡처 (Wireshark)
tcpdump -i lo port 8080 -w cppmmo_packets.pcap

# 연결 상태 확인
netstat -an | grep :8080
```

---

## 🔍 **프로토콜 검증**

### **1. 필수 검증 사항**
- 패킷 ID 유효성 검사
- 필수 필드 null 체크
- 범위 값 검증 (좌표, HP 등)
- 시퀀스 번호 검증

### **2. 오류 처리**
- 잘못된 패킷 형식 처리
- 네트워크 연결 끊김 처리
- 서버 오류 응답 처리

---

*CppMMO Protocol Reference - FlatBuffers 프로토콜 가이드*
*Version: 1.0*