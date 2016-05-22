#include "erpiko/signed-data.h"
#include "erpiko/utils.h"
#include "converters.h"
#include <openssl/pkcs7.h>
#include <openssl/err.h>
#include <iostream>

namespace Erpiko {
class SignedData::Impl {
  public:
    EVP_PKEY *pkey;
    X509 *cert;
    PKCS7 *pkcs7 = nullptr;
    BIO* bio = BIO_new(BIO_s_mem());

    std::unique_ptr<RsaKey> privateKey;
    std::vector<std::unique_ptr<Certificate>> ca;
    std::vector<const Certificate*> caPointer;

    int signerInfoIndex = -1;

    bool success = false;
    bool imported = false;
    int nidKey = 0;
    int nidCert = 0;

    Impl() {
      OpenSSL_add_all_algorithms();
    }

    virtual ~Impl() {
      if (pkey) {
        EVP_PKEY_free(pkey);
      }
      X509_free(cert);
      BIO_free(bio);
      if (pkcs7) {
        PKCS7_free(pkcs7);
      }
    }

    void setup(const Certificate& certificate, const RsaKey& privateKey) {
      pkey = Converters::rsaKeyToPkey(privateKey);
      cert = Converters::certificateToX509(certificate);
    }

    void fromDer(const std::vector<unsigned char> der, const Certificate& certificate) {
      imported = true;
      BIO* mem = BIO_new_mem_buf((void*) der.data(), der.size());
      pkcs7 = d2i_PKCS7_bio(mem, NULL);
      auto ret = (pkcs7 != nullptr) && setSignerInfo(certificate);

      cert = Converters::certificateToX509(certificate);
      if (ret) {
        success = true;
        return;
      }
    }

    bool setSignerInfo(const Certificate& certificate) {
      if (!pkcs7) return false;
      const Identity& id = certificate.subjectIdentity();

      auto signerInfos = pkcs7->d.sign->signer_info;
      if (!signerInfos) {
        return false;
      }

      bool found = false;
      for (int i = 0; i < sk_PKCS7_SIGNER_INFO_num(signerInfos); i ++) {
        auto signerInfo = sk_PKCS7_SIGNER_INFO_value(signerInfos, i);
        auto signerDer = Converters::nameToIdentityDer(signerInfo->issuer_and_serial->issuer);

        if (signerDer == id.toDer()) {
          found = true;
          signerInfoIndex = i;
          break;
        }
      }

      return found;
    }
};

SignedData::SignedData() : impl{std::make_unique<Impl>()} {
}

SignedData::SignedData(const Certificate& certificate, const RsaKey& privateKey) : impl{std::make_unique<Impl>()} {
  impl->setup(certificate, privateKey);
}


SignedData* SignedData::fromDer(const std::vector<unsigned char> der, const Certificate& cert) {
  auto p = new SignedData();

  p->impl->fromDer(der, cert);

  if (!p->impl->success) {
    return nullptr;
  }
  return p;
}

const std::vector<unsigned char> SignedData::toDer() const {
  std::vector<unsigned char> retval;
  int ret;
  BIO* mem = BIO_new(BIO_s_mem());

  ret = i2d_PKCS7_bio_stream(mem, impl->pkcs7, NULL, 0);

  while (ret) {
    unsigned char buff[1024];
    int ret = BIO_read(mem, buff, 1024);
    if (ret > 0) {
      for (int i = 0; i < ret; i ++) {
        retval.push_back(buff[i]);
      }
    } else {
      break;
    }
  }
  BIO_free(mem);

  return retval;

}

SignedData::~SignedData() = default;

bool SignedData::isDetached() const {
  return PKCS7_is_detached(impl->pkcs7);
}

void SignedData::update(const std::vector<unsigned char> data) {
  update(data.data(), data.size());
}

void SignedData::update(const unsigned char* data, const size_t length) {
  BIO_write(impl->bio, data, length);
}

bool SignedData::verify() const {
   STACK_OF(X509) *certs = sk_X509_new_null();
   auto store = X509_STORE_new();
   sk_X509_push(certs, impl->cert);
   auto ret = PKCS7_verify(impl->pkcs7, certs, store, impl->bio, NULL, PKCS7_NOVERIFY) == 1;
   sk_X509_free(certs);
   X509_STORE_free(store);
   return ret == 1;
}

void SignedData::signDetached() {
  if (impl->pkcs7) return;
  impl->pkcs7 = PKCS7_sign(impl->cert, impl->pkey, NULL, impl->bio, PKCS7_DETACHED);
}

void SignedData::sign() {
  if (impl->pkcs7) return;
  impl->pkcs7 = PKCS7_sign(impl->cert, impl->pkey, NULL, impl->bio, 0);
}


} // namespace Erpiko
