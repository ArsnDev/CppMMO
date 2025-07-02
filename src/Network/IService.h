#pragma once

namespace CppMMO
{
    namespace Network
    {
        class IService
        {
        public:
            virtual ~IService() = default;

            virtual void Start() = 0;
            virtual void Stop() = 0;
        };
    }
}
