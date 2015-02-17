#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include <lib/asm.h>

// Length sub specifiers
#define _hh (0x1)
#define _h (0x2)
#define _l (0x4)
#define _ll (0x8)
#define _j (0x10)
#define _z (0x20)
#define _t (0x40)

// Flags sub specifiers
#define LEFTJUSTIFY (0x1)
#define PLUS (0x2)
#define SPACE (0x4)
#define SPECIAL (0x8)
#define ZEROPAD (0x10)

// is_number determines if a characater is a number or not
// get_number returns the number represented by the character
#define is_number(c) ((c) >= '0' && (c) <= '9')
#define get_number(c) (uint8_t)((c) - '0')

// Holds the formatted number
static char fmt_num[256];

// Count the number of digits in the number represented in the string
size_t num_digits(const char *s) {
	size_t num = 0; 
	for(; is_number(*s) && s != '\0'; s++, num++);
	return(num);
}

// Converts a string into a int
int32_t atoi(const char *s) {
	const char *str = s;
	uint8_t negafier = 1;
	if(*str == '-') {
		negafier = -1;
		++str;
	} else {
		if(*str == '+') {
			negafier = 1;
			++str;
		}
	}
	uint32_t digits = num_digits(str);
	if(digits == 0) return(0);
	str += digits-1;

	int32_t integer = 0;
	for(uint32_t multiplier = 1; digits > 0; str--, digits--, multiplier *= 10) {
		integer += get_number(*str)*multiplier;
	}

	return(integer*negafier);
}

// Converts an int into a string
static const char *lower = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char *upper = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char* itoa2(uint32_t num, char *str, uint32_t base, bool upcase) {
	char *buf = str;
	char tmp[36];

	if(base > 36) return(0);
	if(num == 0) *buf++ = '0';

	uint32_t i = 0;
	while(num != 0) {
		uint32_t R = _umod(num, base);
		num = (uint32_t)((double)num / (double)base);
		tmp[i++] = (upcase) ? upper[R] : lower[R];
	}

	while(i-- > 0) *buf++ = tmp[i];	
	
	*buf = '\0';

	return(str);
}

char* number(char *dest, char *str_num, uint8_t flags, uint32_t width,
		int32_t precision, bool negative, char specifier) {
	char *buf = dest;
	char sign = 0;
	if(flags & SPACE) sign = ' ';
	if(flags & PLUS) sign = '+';
	if(negative) sign = '-';

	char padding = ' ';
	if(flags & ZEROPAD) padding = '0';

	char octal = 0;
	char hex[2];
	*hex = '\0';
	if(flags & SPECIAL) {
		if(specifier == 'o') octal = '0';
		else if(specifier == 'x' || specifier == 'p') {
			hex[0] = '0';
			hex[1] = 'x';
		} else if(specifier == 'X') {
			hex[0] = '0';
			hex[1] = 'X';
		}
	}

	size_t len = strlen(str_num);
	size_t working_len = len;
	size_t precision_len = 0;

	if(sign != 0) ++working_len;
	if(octal != 0) ++working_len;
	if(*hex != '\0') working_len += 2;
	if(precision >= 0) {
		if(specifier == 's' && len > (uint32_t)precision) {
			working_len = len = precision;
		} else if(len < (uint32_t)precision) {
			precision_len = precision - len;
			working_len += precision_len;
		}
	}


	if((flags & LEFTJUSTIFY) && padding == ' ') {
		if(sign != 0) *buf++ = sign;
		if(octal != 0) *buf++ = octal;
		if(*hex != '\0') {
			*buf++ = *hex;
			*buf++ = *(hex+1);
		}

		for(uint32_t i = 0; i < precision_len; i++) *buf++ = '0';
		for(size_t i = 0; i < len; i++) *buf++ = str_num[i];
		for(; working_len < width; working_len++) *buf++ = padding;
	} else if(padding == '0') {
		if(sign != 0) *buf++ = sign;
		if(octal != 0) *buf++ = octal;
		if(*hex != '\0') {
			*buf++ = *hex;
			*buf++ = *(hex+1);
		}

		for(uint32_t i = 0; i < precision_len; i++) *buf++ = '0';
		for(; working_len < width; working_len++) *buf++ = padding;
		for(size_t i = 0; i < len; i++) *buf++ = str_num[i];
	} else {
		for(; working_len < width; working_len++) *buf++ = padding;

		if(sign != 0) *buf++ = sign;
		if(octal != 0) *buf++ = octal;
		if(*hex != '\0') {
			*buf++ = *hex;
			*buf++ = *(hex+1);
		}

		for(uint32_t i = 0; i < precision_len; i++) *buf++ = '0';
		for(size_t i = 0; i < len; i++) *buf++ = str_num[i];
	}

	*buf = '\0';

	return(buf);
}

int32_t vsprintf(char *s, const char *fmt, va_list args) {
	char *str;

	for(str = s; *fmt != '\0'; ++fmt) {
		// Simply copy character to buffer if not a format specifier
		if(*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		uint8_t flags = 0;
		uint32_t width = 0;
		int32_t precision = -1;
		uint8_t length;
		char specifier = 0;


		// First come the flags. Get all flags specified
		++fmt;
		for(bool i = false; i != true;) {
			switch(*fmt) {
				case '-': flags |= LEFTJUSTIFY; ++fmt;  break;
				case '+': flags |= PLUS; ++fmt; break;
				case ' ': flags = (flags & PLUS) ? flags : flags|SPACE; ++fmt; break;
				case '#': flags |= SPECIAL; ++fmt; break;
				case '0': flags |= ZEROPAD; ++fmt; break;
				default: i = true; break;
			}
		}

		// Next up is width sub specifier
		if(*fmt == '*') {
			// Next arg is the width sub specifier
			width = va_arg(args, uint32_t);
			++fmt;
		} else if(is_number(*fmt)) {
			// Convert the string into a number
			size_t num = num_digits(fmt);
			width = atoi(fmt);
			fmt += num;
		} else {
			// Otherwise there is no width, turn of ZEROPAD and LEFTJUSTIFY
			width = 0;
			flags &= ~(ZEROPAD | LEFTJUSTIFY);
		}

		if(*fmt == '.') {
			// A precision has been specified
			++fmt;
			if(*fmt == '*') {
				// Next arg is precision value, set ZEROPAD flag
				precision = va_arg(args, uint32_t);
				++fmt;
			} else if(is_number(*fmt)) {
				// Convert string to number, set ZEROPAD flag
				size_t num = num_digits(fmt);
				precision = atoi(fmt);
				fmt += num;
			} else {
				// Otherwise no precision has been specified
				precision = 0;
			}
		}

		// Determine length sub specifier if there is one
		switch(*fmt) {
			case 'h':
				// Check if next character is h
				++fmt;
				if(*fmt == 'h') {
					length = _hh;
					++fmt;
				} else {
					length = _h;
				}
				break;
			case 'l':
				// Check if next character is l
				++fmt;
				if(*fmt == 'l') {
					length = _ll;
					++fmt;
				} else {
					length = _l;
				}
				break;
			case 'j':
				++fmt;
				length = _j;
				break;
			case 'z':
				++fmt;
				length = _z;
				break;
			case 't':
				++fmt;
				length = _t;
				break;
			default:
				break;
		}

		// Now we can check for the specifier character and do the deeds
		switch(*fmt) {
			case 'd':
			case 'i':
			{
				specifier = 'd';

				int32_t num = va_arg(args, int32_t);
				bool negative = false;
				char str_num[32];

				if(precision == 0 && num == 0) break;

				if(num < 0) {
					negative = true;
					num *= -1;
				}
				itoa2(num, str_num, 10, false);

				number(fmt_num, str_num, flags, width, precision, negative, specifier);

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			case 'u':
			{
				specifier = 'u';
				
				uint32_t num = va_arg(args, uint32_t);
				char str_num[32];

				if(precision == 0 && num == 0) break;

				itoa2(num, str_num, 10, false);

				number(fmt_num, str_num, flags, width, precision, false, specifier);

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			case 'o':
			{
				specifier = 'o';

				int32_t num = va_arg(args, int32_t);
				bool negative = false;
				char str_num[32];

				if(precision == 0 && num == 0) break;

				if(num < 0) {
					negative = true;
					num *= -1;
				}
				itoa2(num, str_num, 8, false);

				number(fmt_num, str_num, flags, width, precision, negative, specifier);

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			case 'x':
				specifier = 'x';
			case 'X':
			{
				bool upcase = true;
				if(specifier == 'x') upcase = false;
				else specifier = 'X';

				int32_t num = va_arg(args, int32_t);
				bool negative = false;
				char str_num[32];

				if(precision == 0 && num == 0) break;

				if(num < 0) {
					negative = true;
					num *= -1;
				}
				itoa2(num, str_num, 16, upcase);

				number(fmt_num, str_num, flags, width, precision, negative, specifier);

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			case 'c':
				specifier = 'c';
			case 's':
			{
				flags &= ~(PLUS | SPACE | SPECIAL | ZEROPAD);

				if(specifier == 'c') {
					uint32_t c_int = va_arg(args, uint32_t);
					char c[2];
					c[0] = (char)c_int;
					c[1] = '\0';
					precision = -1;
					number(fmt_num, c, flags, width, precision, false, specifier);
				} else {
					specifier = 's';
					char *string = va_arg(args, char*);
					number(fmt_num, string, flags, width, precision, false, specifier);
				}

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			case 'p':
			{
				specifier = 'p';

				uintptr_t num = va_arg(args, uintptr_t);
				char str_num[32];

				itoa2(num, str_num, 16, false);

				number(fmt_num, str_num, flags, width, precision, false, specifier);

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			case 'n':
			{
				specifier = 'n';

				int32_t *n = va_arg(args, int32_t*);

				*n = (int32_t)(str-s);
				break;
			}
      case 'f':
      {
        specifier = 'f';

        double num = va_arg(args, double);
        char str_num[32];

        uint32_t whole = (uint32_t)num;
        itoa2(whole, str_num, 10, false);
        size_t len = strlen(str_num);
        for(uint32_t i = 0; i < len; i++) *str++ = str_num[i];
        

        uint32_t pow10 = 10;
        for(int32_t i = precision; i > 0; i--) {
          pow10 *= pow10;
        }

        double frac_part = num - (double)((uint32_t)num);
        uint32_t frac = (precision > 6) ? (uint32_t)(frac_part * (double)pow10) : (uint32_t)(frac_part * (double)1000000);
        if(frac == 0 && (flags & SPECIAL)) {
          *str++ = '.';
          *str++ = '0';
        } else {
          *str++ = '.';
          memset(str_num, 32, 0);
          itoa2(frac, str_num, 10, false);
          number(fmt_num, str_num, flags, width, precision, false, specifier);
          len = strlen(fmt_num);
          for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];
        }


        break;
      }
			case '%':
			{
				specifier = '%';

				char c[2];
				c[0] = '%';
				c[1] = '\0';

				flags &= ~(PLUS | SPACE | SPECIAL | ZEROPAD);
				precision = -1;

				number(fmt_num, c, flags, width, precision, false, specifier);

				size_t len = strlen(fmt_num);
				for(uint32_t i = 0; i < len; i++) *str++ = fmt_num[i];

				break;
			}
			default:
				break;
		}
	}

	*str = '\0';

	return(str-s);
}

