#include "commons.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>

BasicParser::BasicParser(std::string filename, std::string source) 
	: filename(filename), source(source), pos(0), line(1), linestart(0) {
}

char BasicParser::peek() {
	if (pos >= source.length()) {
		return '\0';
	}
	return source[pos];
}

char BasicParser::nextChar() {
	if (pos >= source.length()) {
		return '\0';
	}
	char c = source[pos++];
	if (c == '\n') {
		line++;
		linestart = pos;
	}
	return c;
}

bool BasicParser::hasNext() {
	return pos < source.length();
}

void BasicParser::expect(char c) {
	if (peek() != c) {
		throw error("'" + std::string(1, c) + "' expected");
	}
	pos++;
}

void BasicParser::expectNewLine() {
	skipWhitespace();
	if (peek() == '\n') {
		pos++;
		line++;
		linestart = pos;
	} else if (peek() != '\0') {
		throw error("newline expected");
	}
}

void BasicParser::skipWhitespace() {
	while (hasNext() && is_whitespace(peek())) {
		pos++;
	}
}

std::string BasicParser::parseName() {
	std::string name;
	if (!is_identifier_start(peek())) {
		throw error("identifier expected");
	}
	while (hasNext() && is_identifier_char(peek())) {
		name += nextChar();
	}
	return name;
}

double BasicParser::parseNumber(int sign) {
	std::string numstr;
	bool hasDot = false;
	
	if (peek() == '.') {
		numstr += nextChar();
		hasDot = true;
	}
	
	while (hasNext() && (is_digit(peek()) || peek() == '.')) {
		if (peek() == '.') {
			if (hasDot) {
				break;
			}
			hasDot = true;
		}
		numstr += nextChar();
	}
	
	if (peek() == 'e' || peek() == 'E') {
		numstr += nextChar();
		if (peek() == '+' || peek() == '-') {
			numstr += nextChar();
		}
		while (hasNext() && is_digit(peek())) {
			numstr += nextChar();
		}
	}
	
	return sign * std::stod(numstr);
}

std::string BasicParser::parseString(char quote) {
	std::string str;
	bool escaped = false;
	
	// Пропускаем открывающую кавычку
	if (peek() == quote) {
		pos++;
	}
	
	while (hasNext()) {
		char c = peek();
		if (c == '\0') break;
		
		if (escaped) {
			pos++; // Пропускаем escape символ
			c = peek();
			switch (c) {
				case 'n': str += '\n'; pos++; break;
				case 't': str += '\t'; pos++; break;
				case 'r': str += '\r'; pos++; break;
				case '\\': str += '\\'; pos++; break;
				case '"': str += '"'; pos++; break;
				case '\'': str += '\''; pos++; break;
				case 'u': {
					pos++; // Пропускаем 'u'
					// Unicode escape (simplified)
					std::string hex;
					for (int i = 0; i < 4 && hasNext() && isxdigit(peek()); i++) {
						hex += peek();
						pos++;
					}
					if (hex.length() == 4) {
						unsigned int code = std::stoul(hex, nullptr, 16);
						if (code <= 0x7F) {
							str += (char)code;
						} else if (code <= 0x7FF) {
							str += (char)(0xC0 | (code >> 6));
							str += (char)(0x80 | (code & 0x3F));
						} else {
							str += (char)(0xE0 | (code >> 12));
							str += (char)(0x80 | ((code >> 6) & 0x3F));
							str += (char)(0x80 | (code & 0x3F));
						}
					}
					break;
				}
				default: str += c; pos++; break;
			}
			escaped = false;
		} else if (c == '\\') {
			escaped = true;
			pos++;
		} else if (c == quote) {
			pos++; // Пропускаем закрывающую кавычку
			break;
		} else {
			str += c;
			pos++;
		}
	}
	
	return str;
}

std::runtime_error BasicParser::error(std::string message) {
	std::stringstream ss;
	ss << filename << ":" << line << ": " << message;
	return std::runtime_error(ss.str());
}

std::string escape_string(const std::string& str) {
	std::stringstream ss;
	for (char c : str) {
		switch (c) {
			case '\n': ss << "\\n"; break;
			case '\t': ss << "\\t"; break;
			case '\r': ss << "\\r"; break;
			case '\\': ss << "\\\\"; break;
			case '"': ss << "\\\""; break;
			case '\'': ss << "\\'"; break;
			default:
				if (c >= 32 && c < 127) {
					ss << c;
				} else {
					ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << (unsigned int)(unsigned char)c << std::dec;
				}
				break;
		}
	}
	return ss.str();
}

