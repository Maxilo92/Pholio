#pragma once

#include <string_view>

namespace ui::i18n {

enum class Language {
    English,
    German
};

Language languageFromCode(std::string_view code);
const char* languageCode(Language language);

void setLanguage(Language language);
Language getLanguage();

const char* tr(std::string_view key, const char* englishFallback);

} // namespace ui::i18n
