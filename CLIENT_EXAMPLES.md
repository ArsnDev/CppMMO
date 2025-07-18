# CppMMO Client Development Examples

## 📋 Unity 클라이언트 개발 예제

### 🚀 **1. 기본 네트워크 클라이언트**

```csharp
using System;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;
using FlatBuffers;

public class NetworkClient : MonoBehaviour
{
    private TcpClient tcpClient;
    private NetworkStream stream;
    private Thread receiveThread;
    private bool isConnected = false;
    
    [Header("Server Settings")]
    public string serverIP = "localhost";
    public int serverPort = 8080;
    
    void Start()
    {
        ConnectToServer();
    }
    
    void ConnectToServer()
    {
        try
        {
            tcpClient = new TcpClient();
            tcpClient.Connect(serverIP, serverPort);
            stream = tcpClient.GetStream();
            isConnected = true;
            
            // 수신 스레드 시작
            receiveThread = new Thread(ReceiveData);
            receiveThread.Start();
            
            Debug.Log($"Connected to server: {serverIP}:{serverPort}");
        }
        catch (Exception e)
        {
            Debug.LogError($"Connection failed: {e.Message}");
        }
    }
    
    void ReceiveData()
    {
        byte[] buffer = new byte[4096];
        while (isConnected)
        {
            try
            {
                int bytesRead = stream.Read(buffer, 0, buffer.Length);
                if (bytesRead > 0)
                {
                    ProcessReceivedData(buffer, bytesRead);
                }
            }
            catch (Exception e)
            {
                Debug.LogError($"Receive error: {e.Message}");
                break;
            }
        }
    }
    
    void ProcessReceivedData(byte[] data, int length)
    {
        // FlatBuffers 패킷 파싱
        var buffer = new ByteBuffer(data, 0, length);
        var packet = UnifiedPacket.GetRootAsUnifiedPacket(buffer);
        
        // 메인 스레드에서 처리하기 위해 큐에 추가
        MainThreadDispatcher.Instance.Enqueue(() => HandlePacket(packet));
    }
    
    void HandlePacket(UnifiedPacket packet)
    {
        switch (packet.Id)
        {
            case PacketId.S_LoginSuccess:
                HandleLoginSuccess(packet);
                break;
            case PacketId.S_ZoneEntered:
                HandleZoneEntered(packet);
                break;
            case PacketId.S_WorldSnapshot:
                HandleWorldSnapshot(packet);
                break;
            case PacketId.S_PlayerJoined:
                HandlePlayerJoined(packet);
                break;
            case PacketId.S_PlayerLeft:
                HandlePlayerLeft(packet);
                break;
        }
    }
    
    public void SendPacket(byte[] data)
    {
        if (isConnected && stream != null)
        {
            try
            {
                stream.Write(data, 0, data.Length);
            }
            catch (Exception e)
            {
                Debug.LogError($"Send error: {e.Message}");
            }
        }
    }
    
    void OnDestroy()
    {
        isConnected = false;
        receiveThread?.Join(1000);
        stream?.Close();
        tcpClient?.Close();
    }
}
```

### 🔐 **2. 인증 시스템**

```csharp
using FlatBuffers;
using CppMMO.Protocol;

public class AuthenticationManager : MonoBehaviour
{
    [Header("Authentication")]
    public string sessionTicket = "your_session_ticket";
    
    private NetworkClient networkClient;
    private bool isAuthenticated = false;
    
    void Start()
    {
        networkClient = GetComponent<NetworkClient>();
        
        // 서버 연결 후 자동 로그인
        StartCoroutine(WaitForConnectionAndLogin());
    }
    
    IEnumerator WaitForConnectionAndLogin()
    {
        float timeout = 10f; // 10초 타임아웃
        float elapsed = 0f;
        
        while (!networkClient.isConnected && elapsed < timeout)
        {
            yield return new WaitForSeconds(0.1f);
            elapsed += 0.1f;
        }
        
        if (!networkClient.isConnected)
        {
            Debug.LogError("Connection timeout");
            yield break;
        }
        
        yield return new WaitForSeconds(0.5f); // 연결 안정화 대기
        
        SendLoginRequest();
    }
    
    void SendLoginRequest()
    {
        var builder = new FlatBufferBuilder(1024);
        
        // 세션 티켓 문자열 생성
        var sessionTicketOffset = builder.CreateString(sessionTicket);
        
        // C_Login 패킷 생성
        var loginPacket = C_Login.CreateC_Login(builder, sessionTicketOffset);
        
        // UnifiedPacket으로 래핑
        var packet = UnifiedPacket.CreateUnifiedPacket(builder, 
            PacketId.C_Login, 
            Packet.C_Login, 
            loginPacket.Union());
        
        builder.Finish(packet);
        
        // 패킷 전송
        networkClient.SendPacket(builder.SizedByteArray());
        
        Debug.Log("Login request sent");
    }
    
    public void HandleLoginSuccess(UnifiedPacket packet)
    {
        var loginSuccess = packet.Data<S_LoginSuccess>();
        var playerInfo = loginSuccess.PlayerInfo;
        
        isAuthenticated = true;
        
        Debug.Log($"Login successful! Player ID: {playerInfo.PlayerId}, Name: {playerInfo.Name}");
        
        // 플레이어 정보 저장
        GameManager.Instance.SetPlayerInfo(playerInfo);
        
        // 존 입장 요청
        StartCoroutine(EnterZoneAfterDelay());
    }
    
    IEnumerator EnterZoneAfterDelay()
    {
        yield return new WaitForSeconds(1f);
        SendEnterZoneRequest();
    }
    
    void SendEnterZoneRequest()
    {
        var builder = new FlatBufferBuilder(1024);
        
        // C_EnterZone 패킷 생성 (zone_id = 1)
        var enterZonePacket = C_EnterZone.CreateC_EnterZone(builder, 1);
        
        // UnifiedPacket으로 래핑
        var packet = UnifiedPacket.CreateUnifiedPacket(builder, 
            PacketId.C_EnterZone, 
            Packet.C_EnterZone, 
            enterZonePacket.Union());
        
        builder.Finish(packet);
        
        // 패킷 전송
        networkClient.SendPacket(builder.SizedByteArray());
        
        Debug.Log("Enter zone request sent");
    }
}
```

### 🎮 **3. 플레이어 입력 및 이동**

```csharp
using UnityEngine;
using FlatBuffers;
using CppMMO.Protocol;

public class PlayerController : MonoBehaviour
{
    [Header("Movement")]
    public float moveSpeed = 5f;
    public float inputSendRate = 20f; // 초당 입력 전송 횟수
    
    private NetworkClient networkClient;
    private uint sequenceNumber = 0;
    private byte lastInputFlags = 0;
    private float lastInputSendTime = 0f;
    
    void Start()
    {
        networkClient = FindObjectOfType<NetworkClient>();
    }
    
    void Update()
    {
        HandleInput();
        
        // 입력 전송 주기 체크
        if (Time.time - lastInputSendTime >= 1f / inputSendRate)
        {
            SendPlayerInput();
            lastInputSendTime = Time.time;
        }
    }
    
    void HandleInput()
    {
        byte inputFlags = 0;
        
        // WASD 입력 처리
        if (Input.GetKey(KeyCode.W)) inputFlags |= (byte)InputFlags.W;
        if (Input.GetKey(KeyCode.S)) inputFlags |= (byte)InputFlags.S;
        if (Input.GetKey(KeyCode.A)) inputFlags |= (byte)InputFlags.A;
        if (Input.GetKey(KeyCode.D)) inputFlags |= (byte)InputFlags.D;
        if (Input.GetKey(KeyCode.LeftShift)) inputFlags |= (byte)InputFlags.Shift;
        if (Input.GetKey(KeyCode.Space)) inputFlags |= (byte)InputFlags.Space;
        
        // 입력이 변경된 경우에만 즉시 전송
        if (inputFlags != lastInputFlags)
        {
            lastInputFlags = inputFlags;
            SendPlayerInput();
            lastInputSendTime = Time.time;
        }
    }
    
    void SendPlayerInput()
    {
        if (networkClient == null || !networkClient.isConnected)
            return;
        
        var builder = new FlatBufferBuilder(1024);
        
        // 시퀀스 번호 증가
        sequenceNumber++;
        
        // C_PlayerInput 패킷 생성
        var playerInputPacket = C_PlayerInput.CreateC_PlayerInput(builder, 
            lastInputFlags, 
            sequenceNumber);
        
        // UnifiedPacket으로 래핑
        var packet = UnifiedPacket.CreateUnifiedPacket(builder, 
            PacketId.C_PlayerInput, 
            Packet.C_PlayerInput, 
            playerInputPacket.Union());
        
        builder.Finish(packet);
        
        // 패킷 전송
        networkClient.SendPacket(builder.SizedByteArray());
        
        // 디버그 로그 (필요시)
        if (lastInputFlags != 0)
        {
            Debug.Log($"Input sent: Flags={lastInputFlags}, Seq={sequenceNumber}");
        }
    }
    
    // 로컬 예측 이동 (옵션)
    void ApplyLocalMovement()
    {
        Vector3 movement = Vector3.zero;
        
        if ((lastInputFlags & (byte)InputFlags.W) != 0) movement.y += 1f;
        if ((lastInputFlags & (byte)InputFlags.S) != 0) movement.y -= 1f;
        if ((lastInputFlags & (byte)InputFlags.A) != 0) movement.x -= 1f;
        if ((lastInputFlags & (byte)InputFlags.D) != 0) movement.x += 1f;
        
        if (movement.magnitude > 0.1f)
        {
            movement = movement.normalized * moveSpeed * Time.deltaTime;
            transform.position += movement;
        }
    }
}
```

### 🌍 **4. 월드 동기화**

```csharp
using System.Collections.Generic;
using UnityEngine;
using CppMMO.Protocol;

public class WorldManager : MonoBehaviour
{
    [Header("Player Management")]
    public GameObject playerPrefab;
    public Transform playerContainer;
    
    private Dictionary<ulong, GameObject> players = new Dictionary<ulong, GameObject>();
    private ulong myPlayerId = 0;
    
    public void SetMyPlayerId(ulong playerId)
    {
        myPlayerId = playerId;
    }
    
    public void HandleZoneEntered(UnifiedPacket packet)
    {
        var zoneEntered = packet.Data<S_ZoneEntered>();
        var myPlayerInfo = zoneEntered.PlayerInfo;
        
        // 내 플레이어 생성
        CreatePlayer(myPlayerInfo, true);
        
        // 근처 플레이어들 생성
        for (int i = 0; i < zoneEntered.NearbyPlayersLength; i++)
        {
            var nearbyPlayer = zoneEntered.NearbyPlayers(i);
            CreatePlayer(nearbyPlayer, false);
        }
        
        Debug.Log($"Zone entered. My ID: {myPlayerId}, Nearby players: {zoneEntered.NearbyPlayersLength}");
    }
    
    public void HandlePlayerJoined(UnifiedPacket packet)
    {
        var playerJoined = packet.Data<S_PlayerJoined>();
        var playerInfo = playerJoined.PlayerInfo;
        
        CreatePlayer(playerInfo, false);
        
        Debug.Log($"Player joined: {playerInfo.PlayerId}");
    }
    
    public void HandlePlayerLeft(UnifiedPacket packet)
    {
        var playerLeft = packet.Data<S_PlayerLeft>();
        var playerId = playerLeft.PlayerId;
        
        RemovePlayer(playerId);
        
        Debug.Log($"Player left: {playerId}");
    }
    
    public void HandleWorldSnapshot(UnifiedPacket packet)
    {
        var snapshot = packet.Data<S_WorldSnapshot>();
        
        // 모든 플레이어 상태 업데이트
        for (int i = 0; i < snapshot.PlayerStatesLength; i++)
        {
            var playerState = snapshot.PlayerStates(i);
            UpdatePlayerState(playerState);
        }
        
        // 게임 이벤트 처리
        for (int i = 0; i < snapshot.EventsLength; i++)
        {
            var gameEvent = snapshot.Events(i);
            ProcessGameEvent(gameEvent);
        }
    }
    
    void CreatePlayer(PlayerInfo playerInfo, bool isMyPlayer)
    {
        if (players.ContainsKey(playerInfo.PlayerId))
            return;
        
        var playerObj = Instantiate(playerPrefab, playerContainer);
        var playerComponent = playerObj.GetComponent<NetworkedPlayer>();
        
        // 플레이어 정보 설정
        playerComponent.Initialize(playerInfo, isMyPlayer);
        
        // 위치 설정
        var pos = playerInfo.Position;
        playerObj.transform.position = new Vector3(pos.X, pos.Y, pos.Z);
        
        players[playerInfo.PlayerId] = playerObj;
        
        // 내 플레이어인 경우 카메라 추적
        if (isMyPlayer)
        {
            var camera = Camera.main;
            if (camera != null)
            {
                var cameraFollow = camera.GetComponent<CameraFollow>();
                if (cameraFollow != null)
                {
                    cameraFollow.SetTarget(playerObj.transform);
                }
            }
        }
    }
    
    void RemovePlayer(ulong playerId)
    {
        if (players.TryGetValue(playerId, out GameObject playerObj))
        {
            players.Remove(playerId);
            Destroy(playerObj);
        }
    }
    
    void UpdatePlayerState(PlayerState playerState)
    {
        if (players.TryGetValue(playerState.PlayerId, out GameObject playerObj))
        {
            var networkedPlayer = playerObj.GetComponent<NetworkedPlayer>();
            networkedPlayer.UpdateFromServer(playerState);
        }
    }
    
    void ProcessGameEvent(GameEvent gameEvent)
    {
        switch (gameEvent.EventType)
        {
            case EventType.PLAYER_DAMAGE:
                // 플레이어 데미지 처리
                break;
            case EventType.PLAYER_HEAL:
                // 플레이어 힐 처리
                break;
            // 추가 이벤트 처리...
        }
    }
}
```

### 👤 **5. 네트워크 플레이어**

```csharp
using UnityEngine;
using CppMMO.Protocol;

public class NetworkedPlayer : MonoBehaviour
{
    [Header("Player Info")]
    public ulong playerId;
    public string playerName;
    public bool isMyPlayer;
    
    [Header("Movement")]
    public float interpolationSpeed = 10f;
    
    private Vector3 targetPosition;
    private Vector3 targetVelocity;
    private bool isActive;
    
    // UI 컴포넌트
    private TextMesh nameText;
    private SpriteRenderer spriteRenderer;
    
    void Start()
    {
        SetupUI();
    }
    
    void SetupUI()
    {
        // 이름 표시
        var nameObj = new GameObject("PlayerName");
        nameObj.transform.SetParent(transform);
        nameObj.transform.localPosition = Vector3.up * 0.5f;
        
        nameText = nameObj.AddComponent<TextMesh>();
        nameText.text = playerName;
        nameText.anchor = TextAnchor.MiddleCenter;
        nameText.fontSize = 10;
        nameText.color = isMyPlayer ? Color.green : Color.white;
        
        // 스프라이트 렌더러
        spriteRenderer = GetComponent<SpriteRenderer>();
        if (spriteRenderer != null)
        {
            spriteRenderer.color = isMyPlayer ? Color.blue : Color.red;
        }
    }
    
    public void Initialize(PlayerInfo playerInfo, bool myPlayer)
    {
        playerId = playerInfo.PlayerId;
        playerName = playerInfo.Name;
        isMyPlayer = myPlayer;
        
        targetPosition = new Vector3(playerInfo.Position.X, playerInfo.Position.Y, playerInfo.Position.Z);
        transform.position = targetPosition;
    }
    
    public void UpdateFromServer(PlayerState playerState)
    {
        // 서버에서 받은 위치/속도 정보 업데이트
        targetPosition = new Vector3(playerState.Position.X, playerState.Position.Y, playerState.Position.Z);
        targetVelocity = new Vector3(playerState.Velocity.X, playerState.Velocity.Y, playerState.Velocity.Z);
        isActive = playerState.IsActive;
        
        // 비활성 플레이어 처리
        if (!isActive)
        {
            gameObject.SetActive(false);
        }
    }
    
    void Update()
    {
        if (!isActive) return;
        
        // 내 플레이어가 아닌 경우에만 보간
        if (!isMyPlayer)
        {
            InterpolatePosition();
        }
    }
    
    void InterpolatePosition()
    {
        // 위치 보간
        transform.position = Vector3.Lerp(transform.position, targetPosition, 
            Time.deltaTime * interpolationSpeed);
        
        // 속도 기반 예측 (옵션)
        if (targetVelocity.magnitude > 0.1f)
        {
            var predictedPosition = targetPosition + targetVelocity * Time.deltaTime;
            transform.position = Vector3.Lerp(transform.position, predictedPosition, 
                Time.deltaTime * interpolationSpeed * 0.5f);
        }
    }
    
    void OnDrawGizmos()
    {
        // 디버그용 기즈모
        if (isMyPlayer)
        {
            Gizmos.color = Color.blue;
            Gizmos.DrawWireSphere(transform.position, 0.5f);
        }
        
        // 속도 벡터 표시
        if (targetVelocity.magnitude > 0.1f)
        {
            Gizmos.color = Color.yellow;
            Gizmos.DrawLine(transform.position, transform.position + targetVelocity);
        }
    }
}
```

### 🎯 **6. 게임 매니저**

```csharp
using UnityEngine;
using CppMMO.Protocol;

public class GameManager : MonoBehaviour
{
    public static GameManager Instance;
    
    [Header("Game State")]
    public bool isConnected = false;
    public bool isAuthenticated = false;
    public bool isInGame = false;
    
    private NetworkClient networkClient;
    private WorldManager worldManager;
    private PlayerInfo myPlayerInfo;
    
    void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
        }
    }
    
    void Start()
    {
        networkClient = GetComponent<NetworkClient>();
        worldManager = GetComponent<WorldManager>();
        
        // 네트워크 이벤트 등록
        RegisterNetworkEvents();
    }
    
    void RegisterNetworkEvents()
    {
        // 패킷 핸들러 등록
        networkClient.OnPacketReceived += HandlePacket;
    }
    
    void HandlePacket(UnifiedPacket packet)
    {
        switch (packet.Id)
        {
            case PacketId.S_LoginSuccess:
                HandleLoginSuccess(packet);
                break;
            case PacketId.S_LoginFailure:
                HandleLoginFailure(packet);
                break;
            case PacketId.S_ZoneEntered:
                HandleZoneEntered(packet);
                break;
            case PacketId.S_WorldSnapshot:
                HandleWorldSnapshot(packet);
                break;
            case PacketId.S_PlayerJoined:
                HandlePlayerJoined(packet);
                break;
            case PacketId.S_PlayerLeft:
                HandlePlayerLeft(packet);
                break;
        }
    }
    
    void HandleLoginSuccess(UnifiedPacket packet)
    {
        var loginSuccess = packet.Data<S_LoginSuccess>();
        myPlayerInfo = loginSuccess.PlayerInfo;
        
        isAuthenticated = true;
        worldManager.SetMyPlayerId(myPlayerInfo.PlayerId);
        
        Debug.Log($"Login successful! Welcome, {myPlayerInfo.Name}");
        
        // UI 업데이트
        UIManager.Instance.ShowLoginSuccess(myPlayerInfo.Name);
    }
    
    void HandleLoginFailure(UnifiedPacket packet)
    {
        var loginFailure = packet.Data<S_LoginFailure>();
        
        Debug.LogError($"Login failed: {loginFailure.ErrorMessage}");
        
        // UI 업데이트
        UIManager.Instance.ShowLoginError(loginFailure.ErrorMessage);
    }
    
    void HandleZoneEntered(UnifiedPacket packet)
    {
        isInGame = true;
        worldManager.HandleZoneEntered(packet);
        
        // UI 업데이트
        UIManager.Instance.ShowGameUI();
    }
    
    void HandleWorldSnapshot(UnifiedPacket packet)
    {
        worldManager.HandleWorldSnapshot(packet);
    }
    
    void HandlePlayerJoined(UnifiedPacket packet)
    {
        worldManager.HandlePlayerJoined(packet);
    }
    
    void HandlePlayerLeft(UnifiedPacket packet)
    {
        worldManager.HandlePlayerLeft(packet);
    }
    
    public void SetPlayerInfo(PlayerInfo playerInfo)
    {
        myPlayerInfo = playerInfo;
    }
    
    public PlayerInfo GetMyPlayerInfo()
    {
        return myPlayerInfo;
    }
    
    void OnApplicationPause(bool pauseStatus)
    {
        if (pauseStatus)
        {
            // 앱 일시정지 시 연결 유지
            Debug.Log("App paused - maintaining connection");
        }
    }
    
    void OnApplicationFocus(bool hasFocus)
    {
        if (!hasFocus)
        {
            // 포커스 잃었을 때 처리
            Debug.Log("App lost focus");
        }
    }
}
```

### 📱 **7. 메인 스레드 디스패처**

```csharp
using System;
using System.Collections.Generic;
using UnityEngine;

public class MainThreadDispatcher : MonoBehaviour
{
    public static MainThreadDispatcher Instance;
    
    private Queue<Action> actionQueue = new Queue<Action>();
    private readonly object lockObject = new object();
    
    void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
        }
    }
    
    void Update()
    {
        ProcessActions();
    }
    
    public void Enqueue(Action action)
    {
        lock (lockObject)
        {
            actionQueue.Enqueue(action);
        }
    }
    
    void ProcessActions()
    {
        while (actionQueue.Count > 0)
        {
            Action action = null;
            lock (lockObject)
            {
                if (actionQueue.Count > 0)
                {
                    action = actionQueue.Dequeue();
                }
            }
            
            try
            {
                action?.Invoke();
            }
            catch (Exception e)
            {
                Debug.LogError($"Error executing action: {e.Message}");
            }
        }
    }
}
```

---

## 🎯 **개발 순서**

1. **MainThreadDispatcher** → **NetworkClient** → **GameManager** 순으로 구현
2. **AuthenticationManager**로 로그인 기능 구현
3. **PlayerController**로 입력 시스템 구현
4. **WorldManager**와 **NetworkedPlayer**로 멀티플레이어 구현
5. UI 및 게임 완성도 향상

---

## 📞 **주의사항**

- FlatBuffers Unity 패키지 설치 필요
- 네트워크 스레드와 메인 스레드 동기화 주의
- 패킷 파싱 시 null 체크 필수
- 메모리 누수 방지를 위한 적절한 정리 작업

---

*CppMMO Client Examples - Unity 개발 가이드*
*Version: 1.0*