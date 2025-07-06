#include "pch.h"
#include "Utils/Logger.h"
#include "Network/TcpServer.h"
#include "Network/PacketManager.h"
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
        ("threads,t", po::value<int>()->default_value(4), "Set WorkerThread Num.");

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
    int threadCount = vm["threads"].as<int>();

    LOG_INFO("Server configured: Port={}, Threads={}", port, threadCount);

    try
    {
        asio::io_context io_context;

        auto packetManager = std::make_shared<CppMMO::Network::PacketManager>();

        auto server = std::make_shared<CppMMO::Network::TcpServer>(io_context, port, packetManager);

        server->SetOnSessionConnected([](std::shared_ptr<CppMMO::Network::ISession> session)
        {
            LOG_INFO("Client connected: {}", session->GetRemoteEndpoint().address().to_string());
        });

        server->SetOnSessionDisconnected([](std::shared_ptr<CppMMO::Network::ISession> session)
        {
            LOG_INFO("Client disconnected: {}", session->GetRemoteEndpoint().address().to_string());
        });

        CppMMO::Network::ServiceConfig config;
        config.worker_threads = threadCount;
        if (!server->Start(config))
        {
            LOG_CRITICAL("Server failed to start.");
            return 1;
        }

        LOG_INFO("Server started successfully on port {}.", port);

        io_context.run();

        LOG_INFO("Server stopped.");
    }
    catch (const std::exception& e)
    {
        LOG_CRITICAL("Exception occurred in server main loop: {}", e.what());
        return 1;
    }

    return 0;
}