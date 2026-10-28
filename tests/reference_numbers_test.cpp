#include "../pdf_viewer/reference_numbers.h"

#include <cassert>

using namespace section_references;

int main() {
    for (const auto* label : {L"Sec.", L"Sections", L"Subsec.", L"§", L"Ch.", L"Appendix"}) {
        assert(is_section_label(label));
    }
    assert(!is_section_label(L"Figure"));

    for (const auto* title : {L"8.3 Two-Dimensional Waveguides", L"Section 08-03: Waveguides",
                              L"§8.3 Waveguides", L"8.3. Waveguides"}) {
        auto parsed = parse_title(title);
        assert(parsed && parsed->number.key() == L"8.3");
        assert(has_heading_title(title, parsed->end));
    }
    assert(parse_title(L"8.3.1 Details")->number.key() == L"8.3.1");
    assert(parse_title(L"Appendix A.3 Devices")->number.key() == L"A.3");
    assert(parse_title(L"IV.2 Waveguides")->number.key() == L"IV.2");
    assert(parse_title(L"8.3 A General Theory")->number.key() == L"8.3");
    assert(has_heading_title(L"8.3 \u5149\u5b66\u6ce2\u5bfc", parse_title(L"8.3 \u5149\u5b66\u6ce2\u5bfc")->end));
    assert(!parse_title(L"Figure 8.3 Caption"));

    const Number subsection{L"8.5", L'B'};
    for (const auto* heading : {L"B. Coupled Waveguides", L"B) Coupled Waveguides",
                                L"(B) Coupled Waveguides", L"8.5B Coupled Waveguides",
                                L"8.5(B) Coupled Waveguides", L"8.5 B. Coupled Waveguides"}) {
        assert(matches_heading(heading, subsection, true));
    }
    assert(!matches_heading(L"8.3.1 Details", Number{L"8.3"}, false));
    assert(!matches_heading(L"C. Different subsection", subsection, true));
    assert(resolve_outline_child(Number{L"8"}, Number{L"3"}).key() == L"8.3");
    assert(resolve_outline_child(Number{L"8.3"}, Number{L"1"}).key() == L"8.3.1");
    assert(resolve_outline_child(Number{L"8.5"}, Number{L"B"}).key() == L"8.5B");
    assert(resolve_outline_child(Number{L"8"}, Number{L"8.3"}).key() == L"8.3");

    for (const auto* source : {L"see Sec. 8.3", L"Sections 08-03 and 8.4",
                               L"§8.3", L"Subsection 8.5(B)", L"Sec. 8.5 B.",
                               L"Chapter IV.2", L"Appendix A.3"}) {
        std::wcmatch match;
        assert(std::regex_search(source, match, section_reference_regex()));
        assert(parse_title(match.str()));
    }
    assert(!std::regex_search(L"Intersection 8.3", section_reference_regex()));

    assert(equation_key(L"(23.1-11)") == L"23.1-11");
    assert(equation_key(L"(23.1.11)") == L"23.1.11");
    assert(equation_key(L"(08.03A)") == L"8.3A");
    assert(equation_key(L"(8.5(B))") == L"8.5B");
    assert(equation_key(L"[A.2]") == L"A.2");
    assert(!equation_key(L"(not a number)"));
    for (const auto* source : {L"Eq. (23.1-11)", L"Equation 8.5B",
                               L"Equations (8.5(B))", L"Eqn. A.2", L"eq A.2"}) {
        std::wcmatch match;
        assert(std::regex_search(source, match, equation_reference_regex()));
        assert(is_equation_label(match[1].str()));
        assert(equation_key(match[2].str()));
    }
    assert(!std::regex_search(L"Inequations 3.4", equation_reference_regex()));
    for (const auto* tag : {L"(23.1-11)", L"(8.5(B))", L"(8.5 B)",
                            L"(8.5 (B))", L"(A.2)", L"(IV.3)"}) {
        assert(std::regex_match(tag, equation_tag_regex()));
    }
    assert(!std::regex_match(L"[1]", equation_tag_regex()));
}
