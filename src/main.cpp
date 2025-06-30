#include <iostream>
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>


int main()
{
    std::cout << "Hello, CppMMO Server!" << std::endl;
    std::cout << "-----------------------" << std::endl;

    // Boost 버전 확인
    std::cout << "Using Boost version: "
        << BOOST_VERSION / 100000 << "."  // Major version
        << BOOST_VERSION / 100 % 1000 << "." // Minor version
        << BOOST_VERSION % 100 << std::endl; // Patch version

    // Boost.System 기본 기능 확인 (오류 코드 예제)
    boost::system::error_code ec; // 기본값은 성공 (오류 없음)
    std::cout << "Boost.System error_code default value: " << ec.value() << std::endl;
    std::cout << "Boost.System error_code message: " << ec.message() << std::endl;

    // 특정 오류 코드 생성 예시
    boost::system::error_code custom_ec(static_cast<int>(boost::system::errc::address_not_available),
        boost::system::system_category());
    std::cout << "Custom error_code value: " << custom_ec.value() << std::endl;
    std::cout << "Custom error_code message: " << custom_ec.message() << std::endl;

    std::cout << "-----------------------" << std::endl;
    std::cout << "Boost integration test complete." << std::endl;

    return 0;
}
