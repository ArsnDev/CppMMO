#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/JobQueue.h"
#include "Utils/JobProcessor.h"
#include "Network/TcpServer.h"
#include "Network/PacketManager.h"
#include "Network/SessionManager.h"
#include "Game/GameLogicQueue.h"
#include "Game/Managers/GameManager.h"
#include "Game/PacketHandlers/LoginPacketHandler.h"
#include "Game/PacketHandlers/ChatPacketHandler.h"
#include "Game/Managers/ChatManager.h"
#include <boost/program_options.hpp>
#include "protocol_generated.h"

using namespace CppMMO::Protocol;

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    CppMMO::Utils::Logger::Init();

    LOG_INFO("Starting server setup...");

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print Help Message.")
        ("port,p", po::value<unsigned short>()->default_value(8080), "Set Server Port.")
        ("io-threads", po::value<int>()->default_value(2), "Set number of network I/O threads.")
        ("logic-threads", po::value<int>()->default_value(4), "Set number of logic processing threads.");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& e)
    {
        LOG_ERROR("Error parsing command line: {}", e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << desc << std::endl;
        return 1;
    }

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    unsigned short port = vm["port"].as<unsigned short>();
    int ioThreadCount = vm["io-threads"].as<int>();
    int logicThreadCount = vm["logic-threads"].as<int>();

    LOG_INFO("Server configured: Port={}, IO Threads={}, Logic Threads={}", port, ioThreadCount, logicThreadCount);

    try
    {
        asio::io_context io_context;

        auto jobQueue = std::make_shared<CppMMO::Utils::JobQueue>();
        auto packetManager = std::make_shared<CppMMO::Network::PacketManager>(jobQueue);
        auto sessionManager = std::make_shared<CppMMO::Network::SessionManager>();
        auto gameLogicQueue = std::make_shared<CppMMO::Game::GameLogicQueue>();
        auto jobProcessor = std::make_shared<CppMMO::Utils::JobProcessor>(jobQueue, packetManager, gameLogicQueue);
        auto gameManager = std::make_shared<CppMMO::Game::Managers::GameManager>(gameLogicQueue, sessionManager);
        
        jobProcessor->Start(logicThreadCount);
        gameManager->Start();

        packetManager->RegisterHandler(PacketId_C_Login, CppMMO::Game::PacketHandlers::LoginPacketHandler());
        packetManager->RegisterHandler(PacketId_C_Chat, CppMMO::Game::PacketHandlers::ChatPacketHandler());

        auto server = std::make_shared<CppMMO::Network::TcpServer>(io_context, port, packetManager, sessionManager);

        CppMMO::Game::Managers::ChatManager::GetInstance().Initialize(server);

        CppMMO::Network::ServiceConfig config;
        config.worker_threads = ioThreadCount;
        if (!server->Start(config))
        {
            LOG_CRITICAL("Server failed to start.");
            gameManager->Stop();
            jobProcessor->Stop();
            return 1;
        }

        LOG_INFO("Server started successfully on port {}.", port);

        io_context.run();

        gameManager->Stop();
        jobProcessor->Stop();
        LOG_INFO("Server stopped.");
    }
    catch (const std::exception& e)
    {
        LOG_CRITICAL("Exception occurred in server main loop: {}", e.what());
        return 1;
    }

    return 0;
}