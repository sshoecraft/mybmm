
#include <string.h>
#include <ctype.h>

enum EDIT_FUNCS { COLLAPSE, COMPRESS, LOWERCASE, TRIM, UNCOMMENT, UPCASE };

struct {
	char *item;
	short func;
} EDIT_ITEM_INFO[] = {
	{ "COLLAPSE", COLLAPSE },
	{ "COMPRESS", COMPRESS },
	{ "LOWERCASE", LOWERCASE },
	{ "TRIM", TRIM, },
	{ "UNCOMMENT", UNCOMMENT },
	{ "UPCASE", UPCASE },
	{ 0, 0 }
};

static struct {
	unsigned collapse: 1;
	unsigned compress: 1;
	unsigned lowercase: 1;
	unsigned trim: 1;
	unsigned uncomment: 1;
	unsigned upcase: 1;
	unsigned fill: 2;
} edit_funcs;

char *strele(int num,char *delimiter,char *string);

char *stredit(char *string, char *list) {
	static char return_info[1024];
	char edit_list[255], *func, *src, *dest;
	register int x, ele, found;

	/* If string is empty, just return it */
	if (*string == '\0') return string;

	func = (char *)&edit_funcs;
	func[0] = 0;
	ele = 0;
	dest = (char *)&edit_list;
	for(src = list; *src != '\0'; src++) {
		if (!isspace((int)*src))
			*dest++ = toupper(*src);
	}
	*dest = '\0';
	return_info[0] = 0;
	strncat(return_info,string,sizeof(return_info));
	while(1) {
		func = strele(ele,",",edit_list);
		if (!strlen(func)) break;
		found = 0;
		for(x=0; EDIT_ITEM_INFO[x].item; x++) {
			if (strcmp(EDIT_ITEM_INFO[x].item,func)==0) {
				found = 1;
				break;
			}
		}
		if (found) {
			switch(EDIT_ITEM_INFO[x].func) {
				case COLLAPSE:
					edit_funcs.collapse = 1;
					break;
				case COMPRESS:
					edit_funcs.compress = 1;
					break;
				case LOWERCASE:
					edit_funcs.lowercase = 1;
					break;
				case TRIM:
					edit_funcs.trim = 1;
					break;
				case UNCOMMENT:
					edit_funcs.uncomment = 1;
					break;
				case UPCASE:
					edit_funcs.upcase = 1;
					break;
			}
		}
		ele++;
	}
	if (edit_funcs.collapse) {
		for(src = dest = (char *)&return_info; *src != '\0'; src++) {
			if (!isspace((int)*src))
				*dest++ = *src;
		}
		*dest = '\0';
	} else if (edit_funcs.compress) {
		found = 0;
		for(src = dest = (char *)&return_info; *src != '\0'; src++) {
			if (isspace((int)*src)) {
				if (found == 0) {
					*dest++ = ' ';
					found = 1;
				}
			}
			else {
				*dest++ = *src;
				found = 0;

			}
		}
		*dest = '\0';
	}
	if (edit_funcs.trim) {
		src = dest = (char *)&return_info;
		while(isspace((int)*src) && *src != '\t' && *src) src++;
		while(*src != '\0') *dest++ = *src++;
		*dest-- = '\0';
		while((dest >= return_info) && isspace((int)*dest)) dest--;
		*(dest+1) = '\0';
	}
	if (edit_funcs.uncomment) {
		for(src = (char *)&return_info; *src != '\0'; src++) {
			if (*src == '!') {
				*src = '\0';
				break;
			}
		}
	}
	if (edit_funcs.upcase) {
		for(src = (char *)&return_info; *src != '\0'; src++)
			*src = toupper(*src);
	} else if (edit_funcs.lowercase) {
		for(src = (char *)&return_info; *src != '\0'; src++)
			*src = tolower(*src);
	}
	return(return_info);
}
