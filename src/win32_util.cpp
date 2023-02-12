#include "stdafx.h"

#include "logging.h"
#include "win32_util.h"

int wide_to_narrow_string(int codepage, std::wstring_view wide, std::vector<char>& out_buffer)
{
    if(wide.empty())
    {
        return 0;
    }

    // Ignore byte-order marks at the beginning of our UTF-16 text
    const wchar_t little_endian_bom = 0xFFFE;
    const wchar_t big_endian_bom = 0xFEFF;
    const wchar_t byte_order_mark = wide[0];
    size_t start_index = 0;
    if((byte_order_mark == little_endian_bom) || (byte_order_mark == big_endian_bom))
    {
        start_index = 1;
    }

    int bytes_required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                             wide.data() + start_index, wide.length() - start_index,
                                             nullptr, 0, nullptr, nullptr);
    if(bytes_required <= 0)
    {
        return 0;
    }

    out_buffer.resize(bytes_required);
    int bytes_written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                            wide.data() + start_index, wide.length() - start_index,
                                            out_buffer.data(), out_buffer.size(),
                                            nullptr, nullptr);
    assert(bytes_written == bytes_required);
    return bytes_written;
}

int narrow_to_wide_string(int codepage, std::string_view narrow, std::vector<wchar_t>& out_buffer)
{
    int chars_required = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS,
                                             narrow.data(), narrow.length(),
                                             nullptr, 0);
    if(chars_required <= 0)
    {
        return 0;
    }

    out_buffer.resize(chars_required);
    int chars_written = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS,
                                                 narrow.data(), narrow.length(),
                                                 out_buffer.data(), out_buffer.size());
    assert(chars_written == chars_required);
    return chars_written;
}

std::tstring to_tstring(std::string_view string)
{
#ifdef UNICODE
    size_t wide_len = pfc::stringcvt::estimate_utf8_to_wide(string.data(), string.length());
    wchar_t* out_buffer = new wchar_t[wide_len];
    pfc::stringcvt::convert_utf8_to_wide(out_buffer, wide_len, string.data(), string.length());
    std::wstring result = std::wstring(out_buffer);
    delete[] out_buffer;
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return string;
#endif // UNICODE
}

std::tstring to_tstring(const std::string& string)
{
    return to_tstring(std::string_view{string});
}

std::tstring to_tstring(const pfc::string8& string)
{
    return to_tstring(std::string_view{string.c_str(), string.length()});
}

std::string from_tstring(std::tstring_view string)
{
#ifdef UNICODE
    size_t narrow_len = pfc::stringcvt::estimate_wide_to_utf8(string.data(), string.length());
    std::string result(narrow_len, '\0');
    size_t chars_converted = pfc::stringcvt::convert_wide_to_utf8(result.data(), narrow_len, string.data(), string.length());
    result.resize(chars_converted);
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return std::string(buffer, buffer_len);
#endif // UNICODE
}

std::string from_tstring(const std::tstring& string)
{
    return from_tstring(std::tstring_view(string));
}

std::tstring normalise_utf8(std::tstring_view input)
{
    // NOTE: fb2k docs specify that tags are UTF-8 encoded
    if((input.length() == 0) || (input.length() > INT_MAX))
    {
        LOG_WARN("Input string for UTF-8 normalisation is too long, skipping...");
        return std::tstring(input.data(), input.length());
    }

    int required_bytes = NormalizeString(NormalizationKD, input.data(), (int)input.length(), nullptr, 0);
    if(required_bytes <= 0)
    {
        LOG_WARN("Estimated number of bytes required for normalised string is negative, skipping normalisation...");
        return std::tstring(input.data(), input.length());
    }

    size_t buffer_size = (size_t)(required_bytes+1);
    TCHAR* buffer = new TCHAR[buffer_size];
    int normalised_bytes = NormalizeString(NormalizationKD, input.data(), (int)input.length(), buffer, buffer_size);
    if(normalised_bytes <= 0)
    {
        LOG_WARN("Failed to normalise UTF-8 string with error %u. Skipping normalisation...", GetLastError());
        return std::tstring(input.data(), input.length());
    }

    std::tstring result(buffer, normalised_bytes);
    delete[] buffer;

    return result;
}

std::optional<SIZE> GetTextExtents(HDC dc, std::tstring_view string)
{
    SIZE output;
    BOOL success = GetTextExtentPoint32(dc, string.data(), string.length(), &output);
    if(success)
    {
        return output;
    }
    else
    {
        return {};
    }
}

BOOL DrawTextOut(HDC dc, int x, int y, std::tstring_view string)
{
    return TextOut(dc, x, y, string.data(), string.length());
}

