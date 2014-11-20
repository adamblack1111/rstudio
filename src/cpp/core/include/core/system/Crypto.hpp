/*
 * Crypto.hpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_SYSTEM_CRYPTO_HPP
#define CORE_SYSTEM_CRYPTO_HPP

#include <string>
#include <vector>

#include <core/Error.hpp>

namespace rscore {
namespace system {
namespace crypto {
      
void initialize();   

rscore::Error HMAC_SHA1(const std::string& data,
                      const std::string& key,
                      std::vector<unsigned char>* pHMAC);

rscore::Error HMAC_SHA1(const std::string& data,
                      const std::vector<unsigned char>& key,
                      std::vector<unsigned char>* pHMAC);   
   
rscore::Error base64Encode(const std::vector<unsigned char>& data,
                         std::string* pEncoded);   
   
rscore::Error base64Encode(const unsigned char* pData,
                         int len, 
                         std::string* pEncoded);
   
rscore::Error base64Decode(const std::string& data,
                         std::vector<unsigned char>* pDecoded);

rscore::Error rsaInit();

void rsaPublicKey(std::string* pExponent, std::string* pModulo);

rscore::Error rsaPrivateDecrypt(const std::string& pCipherText, std::string* pPlainText);

         
} // namespace crypto
} // namespace system
} // namespace rscore

#endif // CORE_SYSTEM_CRYPTO_HPP

