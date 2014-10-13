/*
 * RErrorCategory.hpp
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

#ifndef R_R_ERROR_CATEGORY_HPP
#define R_R_ERROR_CATEGORY_HPP

#include <boost/system/error_code.hpp>

namespace r {
namespace errc {

enum errc_t {
   Success = 0,
   RHomeNotFound,
   UnsupportedLocale,
   ExpressionParsingError,
   CodeExecutionError,
   SymbolNotFoundError,
   ListElementNotFoundError,
   UnexpectedDataTypeError,
   NoDataAvailableError
};

} // namespace errc
} // namespace r


namespace boost {
namespace system {
template <>
struct is_error_code_enum<r::errc::errc_t>
 { static const bool value = true; };
} // namespace system
} // namespace boost



#include <core/Error.hpp>

namespace r {

const boost::system::error_category& rCategory() ;

namespace errc {

inline boost::system::error_code make_error_code( errc_t e )
{
   return boost::system::error_code( e, rCategory() ); }

inline boost::system::error_condition make_error_condition( errc_t e )
{
   return boost::system::error_condition( e, rCategory() );
}

} // namespace errc


rstudiocore::Error rCodeExecutionError(const std::string& errMsg, 
                                const rstudiocore::ErrorLocation& location);
   
bool isCodeExecutionError(const rstudiocore::Error& error, 
                          std::string* pErrMsg = NULL);
   
// use the error message generated by R for code execution errors,
// otherwise use error.message()
std::string endUserErrorMessage(const rstudiocore::Error& error);
   

} // namespace r


#endif // R_R_ERROR_CATEGORY_HPP

