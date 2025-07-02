#include "pch.h"
#include "PacketManager.h"
#include "ISession.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        void PacketManager::HandlePacket(std::shared_ptr<ISession> session, std::span<const std::byte> data)
        {
            // TODO: 구현
        }
    }
}
