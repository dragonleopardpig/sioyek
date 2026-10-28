#pragma once

#include <cwctype>
#include <optional>
#include <regex>
#include <string>
#include <string_view>

namespace section_references {

struct Number {
    std::wstring base;
    wchar_t suffix = 0;

    std::wstring key() const {
        return base + (suffix ? std::wstring(1, suffix) : L"");
    }
};

struct ParsedNumber {
    Number number;
    size_t end = 0;
};

inline bool digit(wchar_t ch) { return ch >= L'0' && ch <= L'9'; }
inline bool letter(wchar_t ch) { return (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z'); }
inline bool space(wchar_t ch) { return std::iswspace(ch); }

inline std::wstring upper(std::wstring_view value) {
    std::wstring result(value);
    for (auto& ch : result) ch = std::towupper(ch);
    return result;
}

inline std::wstring normalize_digits(std::wstring_view value) {
    size_t first = value.find_first_not_of(L'0');
    return first == std::wstring_view::npos ? L"0" : std::wstring(value.substr(first));
}

inline bool roman(std::wstring_view value) {
    if (value.empty() || value.size() > 12) return false;
    for (wchar_t ch : value) {
        if (std::wstring_view(L"IVXLCDM").find(std::towupper(ch)) == std::wstring_view::npos) return false;
    }
    return true;
}

inline bool is_section_label(std::wstring_view value) {
    while (!value.empty() && (value.back() == L'.' || space(value.back()))) value.remove_suffix(1);
    std::wstring label = upper(value);
    return label == L"SEC" || label == L"SECS" || label == L"SECT" ||
        label == L"SECTS" || label == L"SECTION" || label == L"SECTIONS" ||
        label == L"SUBSEC" || label == L"SUBSECS" ||
        label == L"SUBSECTION" || label == L"SUBSECTIONS" ||
        label == L"CH" || label == L"CHAP" || label == L"CHAPS" ||
        label == L"CHAPTER" || label == L"CHAPTERS" ||
        label == L"APPENDIX" || label == L"APPENDICES" || label == L"§";
}

inline std::optional<ParsedNumber> parse_number(std::wstring_view text, size_t start = 0,
    bool unify_separators = true) {
    size_t i = start;
    while (i < text.size() && space(text[i])) i++;
    const bool parenthesized = i < text.size() && text[i] == L'(';
    if (parenthesized) i++;
    if (i == text.size()) return {};

    Number number;
    size_t component_start = i;
    if (digit(text[i])) {
        while (i < text.size() && digit(text[i])) i++;
        number.base = normalize_digits(text.substr(component_start, i - component_start));
    }
    else if (letter(text[i])) {
        while (i < text.size() && letter(text[i])) i++;
        std::wstring_view component = text.substr(component_start, i - component_start);
        if (component.size() != 1 && !roman(component)) return {};
        number.base = upper(component);
    }
    else return {};

    while (i + 1 < text.size() && (text[i] == L'.' || text[i] == L'-') && digit(text[i + 1])) {
        const wchar_t separator = unify_separators ? L'.' : text[i];
        i++;
        component_start = i;
        while (i < text.size() && digit(text[i])) i++;
        number.base += separator + normalize_digits(text.substr(component_start, i - component_start));
    }

    if (i < text.size() && letter(text[i]) && digit(text[i - 1])) {
        number.suffix = std::towupper(text[i++]);
    }
    else if (i + 1 < text.size() && (text[i] == L'.' || text[i] == L'-') &&
        letter(text[i + 1]) && (i + 2 == text.size() || !letter(text[i + 2]))) {
        number.suffix = std::towupper(text[i + 1]);
        i += 2;
    }
    else {
        size_t j = i;
        while (j < text.size() && space(text[j])) j++;
        if (j + 2 < text.size() && text[j] == L'(' && letter(text[j + 1]) && text[j + 2] == L')') {
            number.suffix = std::towupper(text[j + 1]);
            i = j + 3;
        }
        else if (j > i && j < text.size() && letter(text[j]) &&
            (j + 1 == text.size() || text[j + 1] == L'.' || text[j + 1] == L')')) {
            number.suffix = std::towupper(text[j]);
            i = j + 1;
        }
    }

    if (parenthesized) {
        if (i >= text.size() || text[i] != L')') return {};
        i++;
    }
    if (i < text.size() && (digit(text[i]) || letter(text[i]))) return {};
    return ParsedNumber{number, i};
}

inline std::optional<ParsedNumber> parse_title(std::wstring_view text) {
    size_t i = 0;
    while (i < text.size() && space(text[i])) i++;
    if (i < text.size() && text[i] == L'§') {
        i++;
    }
    else if (i < text.size() && letter(text[i])) {
        size_t label_start = i;
        while (i < text.size() && letter(text[i])) i++;
        if (is_section_label(text.substr(label_start, i - label_start))) {
            if (i < text.size() && text[i] == L'.') i++;
        }
        else i = label_start;
    }
    while (i < text.size() && (space(text[i]) || text[i] == L':')) i++;
    return parse_number(text, i);
}

inline bool matches_heading(std::wstring_view line, const Number& target, bool allow_letter_only) {
    auto parsed = parse_title(line);
    if (!parsed) return false;
    if (parsed->number.key() == target.key()) return true;
    return allow_letter_only && target.suffix &&
        parsed->number.base == std::wstring(1, target.suffix) && !parsed->number.suffix;
}

inline Number resolve_outline_child(const Number& parent, Number child) {
    if (parent.base.empty() || child.base == parent.base ||
        child.base.find(L'.') != std::wstring::npos || child.suffix) return child;
    if (child.base.size() == 1 && letter(child.base[0])) {
        child.suffix = child.base[0];
        child.base = parent.base;
    }
    else {
        child.base = parent.base + L'.' + child.base;
    }
    return child;
}

inline bool has_heading_title(std::wstring_view line, size_t after_number) {
    if (line.find(L"....") != std::wstring_view::npos) return false;
    for (size_t i = after_number; i < line.size(); i++) {
        if (letter(line[i]) || (line[i] > 127 && !space(line[i]) && !std::iswpunct(line[i]))) return true;
    }
    return false;
}

inline const std::wstring& section_reference_pattern() {
    static const std::wstring pattern =
        LR"(((\b(Subsections?|Subsecs?|Sections?|Sects?|Secs?|Chapters?|Chaps?|Ch|Appendices|Appendix)\.?)|§)[ \t]*([A-Za-z0-9]+([.\-][A-Za-z0-9]+)*(\([A-Za-z]\)|[ \t]+\([A-Za-z]\)|[ \t]+[A-Za-z]\b)?))";
    return pattern;
}

inline const std::wregex& section_reference_regex() {
    static const std::wregex regex(section_reference_pattern(), std::regex_constants::icase);
    return regex;
}

inline bool is_equation_label(std::wstring_view value) {
    while (!value.empty() && (value.back() == L'.' || space(value.back()))) value.remove_suffix(1);
    std::wstring label = upper(value);
    return label == L"EQ" || label == L"EQS" || label == L"EQN" || label == L"EQNS" ||
        label == L"EQUATION" || label == L"EQUATIONS";
}

inline const std::wstring& equation_reference_pattern() {
    static const std::wstring pattern = LR"(\b(Equations?|Eqns?|Eqs?)\.?[ \t]*([0-9A-Za-z.()\-]+))";
    return pattern;
}

inline const std::wregex& equation_reference_regex() {
    static const std::wregex regex(equation_reference_pattern(), std::regex_constants::icase);
    return regex;
}

inline std::optional<std::wstring> equation_key(std::wstring_view text) {
    while (!text.empty() && space(text.front())) text.remove_prefix(1);
    while (!text.empty() && space(text.back())) text.remove_suffix(1);
    if (text.size() >= 2 && ((text.front() == L'(' && text.back() == L')') ||
        (text.front() == L'[' && text.back() == L']'))) {
        text.remove_prefix(1);
        text.remove_suffix(1);
    }
    auto parsed = parse_number(text, 0, false);
    if (!parsed || parsed->end != text.size()) return {};
    return parsed->number.key();
}

inline const std::wstring& equation_tag_pattern() {
    static const std::wstring pattern =
        LR"(\(([0-9]+|[IVXLCDMivxlcdm]+|[A-Za-z])([.\-][0-9]+)*([A-Za-z]|[ \t]+[A-Za-z]|[ \t]*\([A-Za-z]\))?\))";
    return pattern;
}

inline const std::wregex& equation_tag_regex() {
    static const std::wregex regex(equation_tag_pattern());
    return regex;
}

} // namespace section_references
