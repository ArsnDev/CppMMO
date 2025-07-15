# Stage 1: .NET 애플리케이션 빌드
# 공식 .NET SDK 8.0 이미지를 사용하여 애플리케이션을 빌드합니다.
FROM mcr.microsoft.com/dotnet/sdk:8.0 AS build

# 작업 디렉토리 설정
WORKDIR /src

# 프로젝트 파일 (.csproj)만 먼저 복사하여 종속성 복원 시 Docker 레이어 캐싱을 활용합니다.
# 이는 소스 코드가 변경될 때마다 전체 종속성을 다시 다운로드하는 것을 방지합니다.
COPY ["AuthServer.csproj", "./"]

# 종속성 복원
RUN dotnet restore "AuthServer.csproj"

# 나머지 애플리케이션 소스 코드 복사
# 이전 단계에서 복사된 .csproj 파일과 동일한 디렉토리로 복사합니다.
COPY . "./"

# AuthServer 프로젝트 디렉토리로 이동
WORKDIR "/src"

# 애플리케이션 빌드 및 게시
# 릴리스 모드로 빌드하고, 최종 실행 파일을 /app/publish 경로에 게시합니다.
# /p:UseAppHost=false는 컨테이너 환경에서 불필요한 단일 실행 파일 생성을 방지합니다.
RUN dotnet publish "AuthServer.csproj" -c Release -o /app/publish /p:UseAppHost=false

# Stage 2: 최소한의 런타임 이미지 생성 및 애플리케이션 실행
# 공식 .NET ASP.NET 런타임 8.0 이미지를 사용하여 최종 런타임 환경을 구성합니다.
FROM mcr.microsoft.com/dotnet/aspnet:8.0 AS final

# 작업 디렉토리 설정
WORKDIR /app

# 빌드 스테이지에서 게시된 애플리케이션 파일만 최종 이미지로 복사합니다.
# 이는 최종 이미지 크기를 최소화하는 데 중요합니다.
COPY --from=build /app/publish .

# AuthServer 애플리케이션이 리스닝할 포트(기본 5278)를 노출합니다.
EXPOSE 5278
# 컨테이너 시작 시 실행될 명령을 정의합니다.
# 게시된 AuthServer.dll 파일을 .NET 런타임으로 실행합니다.
ENTRYPOINT ["dotnet", "AuthServer.dll"]