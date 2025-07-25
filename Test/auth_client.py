#!/usr/bin/env python3
"""
AuthServer에서 유효한 세션 티켓을 받아오는 클라이언트
"""
import requests
import json

AUTH_SERVER_URL = "http://localhost:5278"

def register_test_user(username, password):
    """테스트 사용자 등록"""
    url = f"{AUTH_SERVER_URL}/api/auth/register"
    data = {
        "username": username,
        "password": password
    }
    
    response = requests.post(url, json=data)
    print(f"회원가입 응답: {response.status_code}")
    if response.status_code == 200:
        result = response.json()
        print(f"회원가입 성공: {result}")
        return True
    else:
        print(f"회원가입 실패: {response.text}")
        return False

def login_user(username, password):
    """사용자 로그인하여 세션 티켓 받기"""
    url = f"{AUTH_SERVER_URL}/api/auth/login"
    data = {
        "username": username,
        "password": password
    }
    
    response = requests.post(url, json=data)
    print(f"로그인 응답: {response.status_code}")
    if response.status_code == 200:
        result = response.json()
        if result.get('success'):
            print(f"로그인 성공!")
            print(f"세션 티켓: {result.get('sessionTicket')}")
            return result.get('sessionTicket')
        else:
            print(f"로그인 실패: {result.get('message')}")
            return None
    else:
        print(f"로그인 요청 실패: {response.text}")
        return None

def get_characters(session_ticket):
    """캐릭터 목록 조회"""
    url = f"{AUTH_SERVER_URL}/api/auth/characters"
    headers = {
        "sessionTicket": session_ticket
    }
    
    response = requests.get(url, headers=headers)
    print(f"캐릭터 목록 응답: {response.status_code}")
    if response.status_code == 200:
        result = response.json()
        if result.get('success'):
            characters = result.get('characters', [])
            print(f"캐릭터 개수: {len(characters)}")
            for char in characters:
                print(f"  - PlayerId: {char.get('playerId')}, Name: {char.get('name')}")
            return characters
        else:
            print(f"캐릭터 조회 실패: {result.get('message')}")
            return []
    else:
        print(f"캐릭터 조회 요청 실패: {response.text}")
        return []

def create_character(session_ticket, character_name):
    """캐릭터 생성"""
    url = f"{AUTH_SERVER_URL}/api/auth/characters"
    headers = {
        "sessionTicket": session_ticket
    }
    data = {
        "CharacterName": character_name
    }
    
    response = requests.post(url, json=data, headers=headers)
    print(f"캐릭터 생성 응답: {response.status_code}")
    if response.status_code == 200:
        result = response.json()
        if result.get('success'):
            character = result.get('character')
            print(f"캐릭터 생성 성공!")
            print(f"  - PlayerId: {character.get('playerId')}")
            print(f"  - Name: {character.get('name')}")
            return character
        else:
            print(f"캐릭터 생성 실패: {result.get('message')}")
            return None
    else:
        print(f"캐릭터 생성 요청 실패: {response.text}")
        return None

def main():
    print("=== AuthServer 테스트 ===")
    
    username = "loadtest_user"
    password = "password123"
    character_name = "LoadTestChar"
    
    # 1. 회원가입 (이미 있으면 실패해도 OK)
    print("\n1. 회원가입 시도...")
    register_test_user(username, password)
    
    # 2. 로그인
    print("\n2. 로그인...")
    session_ticket = login_user(username, password)
    if not session_ticket:
        print("로그인 실패로 종료")
        return
    
    # 3. 캐릭터 목록 조회
    print("\n3. 캐릭터 목록 조회...")
    characters = get_characters(session_ticket)
    
    # 4. 캐릭터가 없으면 생성
    if not characters:
        print("\n4. 캐릭터 생성...")
        character = create_character(session_ticket, character_name)
        if character:
            characters = [character]
    
    # 5. 결과 출력
    if characters:
        print(f"\n=== 부하 테스트용 정보 ===")
        print(f"세션 티켓: {session_ticket}")
        print(f"플레이어 ID: {characters[0].get('playerId')}")
        print(f"캐릭터 이름: {characters[0].get('name')}")
        
        # 파일로 저장
        test_data = {
            "session_ticket": session_ticket,
            "player_id": characters[0].get('playerId'),
            "character_name": characters[0].get('name')
        }
        
        with open('test_credentials.json', 'w') as f:
            json.dump(test_data, f, indent=2)
        
        print(f"정보가 test_credentials.json에 저장되었습니다.")
    else:
        print("캐릭터 정보를 가져올 수 없습니다.")

if __name__ == "__main__":
    main()