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
#include "Game/Services/AuthService.h"
#include <boost/program_options.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include "protocol_generated.h"

namespace asio = boost::asio; 

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
        ("logic-threads", po::value<int>()->default_value(4), "Set number of logic processing threads.")
        ("server-config", po::value<std::string>()->default_value("config/server_config.json"), "Server configuration file path.");

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
    std::string serverConfigPath = vm["server-config"].as<std::string>();

    // Load server configuration
    std::string authHost = "localhost";
    std::string authPort = "5278";
    
    try {
        std::ifstream serverConfigFile(serverConfigPath);
        if (serverConfigFile.is_open()) {
            nlohmann::json serverConfig;
            serverConfigFile >> serverConfig;
            
            authHost = serverConfig["auth_server"]["host"].get<std::string>();
            authPort = std::to_string(serverConfig["auth_server"]["port"].get<int>());
            
            LOG_INFO("Server config loaded from: {}", serverConfigPath);
        } else {
            LOG_WARN("Could not open server config file: {}, using defaults", serverConfigPath);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load server config: {}, using defaults", e.what());
    }

    LOG_INFO("Server configured: Port={}, IO Threads={}, Logic Threads={}", port, ioThreadCount, logicThreadCount);
    LOG_INFO("Auth Service configured: Host={}, Port={}", authHost, authPort);

    try
    {
        asio::io_context io_context;

        auto jobQueue = std::make_shared<CppMMO::Utils::JobQueue>();
        auto packetManager = std::make_shared<CppMMO::Network::PacketManager>(jobQueue);
        auto gameLogicQueue = std::make_shared<CppMMO::Game::GameLogicQueue>();
        auto sessionManager = std::make_shared<CppMMO::Network::SessionManager>(gameLogicQueue);
        auto jobProcessor = std::make_shared<CppMMO::Utils::JobProcessor>(jobQueue, packetManager, gameLogicQueue);
        auto gameManager = std::make_shared<CppMMO::Game::Managers::GameManager>(gameLogicQueue, sessionManager);
        auto authService = std::make_shared<CppMMO::Game::Services::AuthService>(io_context, authHost, authPort);

        jobProcessor->Start(logicThreadCount);
        gameManager->Start();

        auto loginHandlerInstance = std::make_shared<CppMMO::Game::PacketHandlers::LoginPacketHandler>(io_context, authService);
        packetManager->RegisterHandler(CppMMO::Protocol::PacketId_C_Login,
            [loginHandlerInstance](std::shared_ptr<CppMMO::Network::ISession> session, const CppMMO::Protocol::UnifiedPacket* unifiedPacket) {
                (*loginHandlerInstance)(session, unifiedPacket); 
            });
        auto chatHandlerInstance = std::make_shared<CppMMO::Game::PacketHandlers::ChatPacketHandler>();
        packetManager->RegisterHandler(CppMMO::Protocol::PacketId_C_Chat,
            [chatHandlerInstance](std::shared_ptr<CppMMO::Network::ISession> session, const CppMMO::Protocol::UnifiedPacket* unifiedPacket) {
                (*chatHandlerInstance)(session, unifiedPacket);
            }); 

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