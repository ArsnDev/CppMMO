#!/usr/bin/env python3
"""
대량 부하 테스트를 위한 테스트 계정 생성
"""
import requests
import json
import time
from concurrent.futures import ThreadPoolExecutor

AUTH_SERVER_URL = "http://localhost:5278"
NUM_ACCOUNTS = 100  # 100개 계정으로 1000개 클라이언트 지원

def create_test_account(account_id):
    """단일 테스트 계정 생성"""
    username = f"loadtest_{account_id:03d}"
    password = "password123"
    character_name = f"TestChar{account_id:03d}"
    
    try:
        # 1. 회원가입
        register_url = f"{AUTH_SERVER_URL}/api/auth/register"
        register_data = {"username": username, "password": password}
        register_response = requests.post(register_url, json=register_data, timeout=10)
        
        if register_response.status_code not in [200, 409]:  # 200=성공, 409=이미 존재
            print(f"[{account_id:03d}] 회원가입 실패: {register_response.status_code}")
            return None
        
        # 2. 로그인
        login_url = f"{AUTH_SERVER_URL}/api/auth/login"
        login_data = {"username": username, "password": password}
        login_response = requests.post(login_url, json=login_data, timeout=10)
        
        if login_response.status_code != 200:
            print(f"[{account_id:03d}] 로그인 실패: {login_response.status_code}")
            return None
        
        login_result = login_response.json()
        if not login_result.get('success'):
            print(f"[{account_id:03d}] 로그인 응답 실패: {login_result.get('message')}")
            return None
        
        session_ticket = login_result.get('sessionTicket')
        
        # 3. 캐릭터 목록 조회
        characters_url = f"{AUTH_SERVER_URL}/api/auth/characters"
        headers = {"sessionTicket": session_ticket}
        characters_response = requests.get(characters_url, headers=headers, timeout=10)
        
        if characters_response.status_code != 200:
            print(f"[{account_id:03d}] 캐릭터 조회 실패: {characters_response.status_code}")
            return None
        
        characters_result = characters_response.json()
        characters = characters_result.get('characters', [])
        
        # 4. 캐릭터가 없으면 생성
        if not characters:
            create_url = f"{AUTH_SERVER_URL}/api/auth/characters"
            create_data = {"CharacterName": character_name}
            create_response = requests.post(create_url, json=create_data, headers=headers, timeout=10)
            
            if create_response.status_code != 200:
                print(f"[{account_id:03d}] 캐릭터 생성 실패: {create_response.status_code}")
                return None
            
            create_result = create_response.json()
            if not create_result.get('success'):
                print(f"[{account_id:03d}] 캐릭터 생성 응답 실패: {create_result.get('message')}")
                return None
            
            character = create_result.get('character')
        else:
            character = characters[0]
        
        account_info = {
            "account_id": account_id,
            "username": username,
            "session_ticket": session_ticket,
            "player_id": character.get('playerId'),
            "character_name": character.get('name')
        }
        
        print(f"[{account_id:03d}] 계정 생성 완료: PlayerId={character.get('playerId')}")
        return account_info
        
    except Exception as e:
        print(f"[{account_id:03d}] 예외 발생: {e}")
        return None

def create_all_test_accounts():
    print(f"=== {NUM_ACCOUNTS}개 테스트 계정 생성 시작 ===")
    
    accounts = []
    failed_count = 0
    
    # 병렬로 계정 생성 (10개씩)
    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = []
        for i in range(1, NUM_ACCOUNTS + 1):
            future = executor.submit(create_test_account, i)
            futures.append(future)
        
        for i, future in enumerate(futures):
            try:
                result = future.result(timeout=30)
                if result:
                    accounts.append(result)
                else:
                    failed_count += 1
                    
                # 진행 상황 출력
                if (i + 1) % 10 == 0:
                    print(f"진행률: {i + 1}/{NUM_ACCOUNTS} ({((i + 1) / NUM_ACCOUNTS) * 100:.1f}%)")
                    
            except Exception as e:
                print(f"계정 생성 중 예외: {e}")
                failed_count += 1
    
    print(f"\n=== 계정 생성 완료 ===")
    print(f"성공: {len(accounts)}개")
    print(f"실패: {failed_count}개")
    
    if accounts:
        # 파일로 저장
        with open('load_test_accounts.json', 'w') as f:
            json.dump(accounts, f, indent=2)
        
        print(f"계정 정보가 load_test_accounts.json에 저장되었습니다.")
        
        # 샘플 출력
        print(f"\n샘플 계정 정보:")
        for account in accounts[:5]:
            print(f"  - {account['username']}: PlayerId={account['player_id']}")
    
    return accounts

if __name__ == "__main__":
    create_all_test_accounts()