#include "network/tls.hpp"
#include "io/file_manager.hpp"

TLSConnection::TLSConnection() : m_initialized(false)
{
#ifdef ENABLE_CRYPTO_OPENSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    m_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ctx) {
        Log::error("TLSConnection", "Failed to create SSL context");
        return;
    }
    
    // Load certificate bundle
    if (!SSL_CTX_load_verify_locations(m_ctx, 
        file_manager->getCertBundleLocation().c_str(), nullptr)) {
        Log::error("TLSConnection", "Failed to load certificate bundle");
        return;
    }
    
    // Add certificate store status logging
    X509_STORE* store = SSL_CTX_get_cert_store(m_ctx);
    if (!store) {
        Log::error("TLSConnection", "No certificate store available");
    } else {
        Log::debug("TLSConnection", "Certificate store initialized");
        // Optional: Log more certificate store details
        X509_STORE_CTX* store_ctx = X509_STORE_CTX_new();
        if (store_ctx) {
            int num_certs = 0;
            // Count trusted certificates
            X509_STORE_CTX_init(store_ctx, store, nullptr, nullptr);
            STACK_OF(X509_OBJECT)* objs = X509_STORE_get0_objects(store);
            num_certs = sk_X509_OBJECT_num(objs);
            Log::debug("TLSConnection", "Number of certificates in store: %d", num_certs);
            X509_STORE_CTX_free(store_ctx);
        }
    }
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    mbedtls_ssl_init(&m_ssl);
    mbedtls_ssl_config_init(&m_conf);
    mbedtls_entropy_init(&m_entropy);
    mbedtls_ctr_drbg_init(&m_ctr_drbg);
    mbedtls_net_init(&m_server_fd);
    
    if (mbedtls_ssl_config_defaults(&m_conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        Log::error("TLSConnection", "Failed to set SSL config defaults");
        return;
    }
    
    if (mbedtls_ctr_drbg_seed(&m_ctr_drbg, mbedtls_entropy_func, &m_entropy,
        nullptr, 0) != 0) {
        Log::error("TLSConnection", "Failed to seed RNG");
        return;
    }
    
    mbedtls_ssl_conf_authmode(&m_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&m_conf, mbedtls_ctr_drbg_random, &m_ctr_drbg);
    
    if (mbedtls_ssl_setup(&m_ssl, &m_conf) != 0) {
        Log::error("TLSConnection", "Failed to setup SSL");
        return;
    }
#endif
}

TLSConnection::~TLSConnection()
{
    disconnect();
#ifdef ENABLE_CRYPTO_OPENSSL
    if (m_ctx) SSL_CTX_free(m_ctx);
    EVP_cleanup();
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    mbedtls_net_free(&m_server_fd);
    mbedtls_ssl_free(&m_ssl);
    mbedtls_ssl_config_free(&m_conf);
    mbedtls_ctr_drbg_free(&m_ctr_drbg);
    mbedtls_entropy_free(&m_entropy);
#endif
}

bool TLSConnection::connect(const SocketAddress& addr)
{
#ifdef ENABLE_CRYPTO_OPENSSL
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        Log::error("TLSConnection", "Failed to create socket: %s", strerror(errno));
        return false;
    }
    
    Log::debug("TLSConnection", "Attempting to connect to %s", addr.toString().c_str());
    if (::connect(m_socket, addr.getSockaddr(), addr.getSocklen()) != 0) {
        Log::error("TLSConnection", "Failed to connect socket: %s", strerror(errno));
        return false;
    }
    
    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) {
        Log::error("TLSConnection", "SSL_new failed: %s", 
                   ERR_error_string(ERR_get_error(), nullptr));
        return false;
    }

    if (SSL_set_fd(m_ssl, m_socket) != 1) {
        Log::error("TLSConnection", "SSL_set_fd failed: %s", 
                   ERR_error_string(ERR_get_error(), nullptr));
        return false;
    }
    
    Log::debug("TLSConnection", "Starting SSL handshake");
    int ret = SSL_connect(m_ssl);
    if (ret != 1) {
        int err = SSL_get_error(m_ssl, ret);
        Log::error("TLSConnection", "SSL handshake failed: %s (error code: %d)", 
                   ERR_error_string(ERR_get_error(), nullptr), err);
        return false;
    }

    if (SSL_connect(m_ssl) == 1) {
        Log::debug("TLSConnection", "SSL Connection established:");
        Log::debug("TLSConnection", "  Protocol: %s", SSL_get_version(m_ssl));
        Log::debug("TLSConnection", "  Cipher: %s", SSL_get_cipher(m_ssl));
        
        X509* cert = SSL_get_peer_certificate(m_ssl);
        if (cert) {
            char* subject = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
            Log::debug("TLSConnection", "  Server certificate subject: %s", subject);
            OPENSSL_free(subject);
            X509_free(cert);
        } else {
            Log::warn("TLSConnection", "  No server certificate received");
        }
    }
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", addr.getPort());
    
    if (mbedtls_net_connect(&m_server_fd, addr.getIP().c_str(), port_str,
        MBEDTLS_NET_PROTO_TCP) != 0) {
        Log::error("TLSConnection", "Failed to connect socket");
        return false;
    }
    
    mbedtls_ssl_set_bio(&m_ssl, &m_server_fd,
        mbedtls_net_send, mbedtls_net_recv, nullptr);
    
    int ret;
    while ((ret = mbedtls_ssl_handshake(&m_ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && 
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            Log::error("TLSConnection", "SSL handshake failed");
            return false;
        }
    }
#endif

    m_initialized = true;
    return true;
}

void TLSConnection::disconnect()
{
    if (!m_initialized) return;
    
#ifdef ENABLE_CRYPTO_OPENSSL
    if (m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    mbedtls_ssl_close_notify(&m_ssl);
    mbedtls_net_free(&m_server_fd);
#endif

    m_initialized = false;
}

bool TLSConnection::sendData(const std::string& data)
{
    if (!m_initialized) return false;
    
#ifdef ENABLE_CRYPTO_OPENSSL
    return SSL_write(m_ssl, data.c_str(), data.length()) > 0;
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    int ret;
    while ((ret = mbedtls_ssl_write(&m_ssl, 
        (const unsigned char*)data.c_str(), data.length())) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && 
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false;
        }
    }
    return true;
#endif
}

bool TLSConnection::receiveData(std::string& data, size_t length)
{
    if (!m_initialized) return false;
    
    std::vector<char> buffer(length);
#ifdef ENABLE_CRYPTO_OPENSSL
    int received = SSL_read(m_ssl, buffer.data(), length);
    if (received <= 0) return false;
    data.assign(buffer.data(), received);
#elif defined(ENABLE_CRYPTO_MBEDTLS)
    int ret;
    while ((ret = mbedtls_ssl_read(&m_ssl, 
        (unsigned char*)buffer.data(), length)) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && 
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return false;
        }
    }
    data.assign(buffer.data(), ret);
#endif
    return true;
}
