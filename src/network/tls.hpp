#ifndef HEADER_TLS_HPP
#define HEADER_TLS_HPP

#include "network/socket_address.hpp"
#include "utils/log.hpp"
#include <string>
#include <vector>
#include <memory>

#ifdef ENABLE_CRYPTO_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#elif defined(ENABLE_CRYPTO_MBEDTLS)
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#endif

class TLSConnection {
private:
#ifdef ENABLE_CRYPTO_OPENSSL
    SSL_CTX* m_ctx;
    SSL* m_ssl;
    int m_socket;
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    mbedtls_ssl_context m_ssl;
    mbedtls_ssl_config m_conf;
    mbedtls_entropy_context m_entropy;
    mbedtls_ctr_drbg_context m_ctr_drbg;
    mbedtls_net_context m_server_fd;
#endif
    bool m_initialized;

public:
    TLSConnection();
    ~TLSConnection();
    
    bool connect(const SocketAddress& addr);
    void disconnect();
    bool sendData(const std::string& data);
    bool receiveData(std::string& data, size_t length);
    bool isConnected() const { return m_initialized; }
};

#endif
