#include "parse.h"
#define default_header_capacity 16
extern FILE *yyin;
#include "y.tab.h"
extern void yyrestart(FILE *);

/**
* Given a char buffer returns the parsed request headers
*/
Request * parse(char *buffer, int size) {
  //Differant states in the state machine
	enum {
		STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
	};

	int i = 0, state;
	size_t offset = 0;
	char ch;
	char buf[8192];
	memset(buf, 0, 8192);

	state = STATE_START;
	while (state != STATE_CRLFCRLF) {
		char expected = 0;

		if (i == size)
			break;

		ch = buffer[i++];
		buf[offset++] = ch;

		switch (state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;

	}

    //Valid End State
	if (state == STATE_CRLFCRLF) {
		Request *request = (Request *) malloc(sizeof(Request));
        request->header_count = 0;
        // TODO You will need to handle resizing this in parser.y
        // 给headers设置一个初始容量，如果初始容量还不够大，则在parser.y里翻倍的扩容
		request->header_capacity = default_header_capacity;
		request->headers = (Request_header *) malloc(sizeof(Request_header)*request->header_capacity);
		set_parsing_options(buf, i, request);

		if (yyparse() == SUCCESS) {
            return request;
		}
		else{
			yyrestart(yyin); //输入文件重置
			free(request->headers);
			free(request);
		}
	}
    //TODO Handle Malformed Requests
    printf("Parsing Failed\n");
	return NULL;
}

Requests * chunked_parse(char *buffer, int size) {
  //Differant states in the state machine
	enum {
		STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
	};

	int i = 0, state;
	size_t offset = 0;
	char ch;
	char buf[8192];
	memset(buf, 0, 8192);

	state = STATE_START;
	while (state != STATE_CRLFCRLF) {
		char expected = 0;

		if (i == size)
			break;

		ch = buffer[i++];
		buf[offset++] = ch;

		switch (state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;

	}
	
	//Requests *request = (Requests *) malloc(sizeof(Requests));
    //Valid End State
	if (state == STATE_CRLFCRLF) {
		Requests *request = (Requests *) malloc(sizeof(Requests));
		request->current_request = (Request *) malloc(sizeof(Request));
        request->current_request->header_count = 0;
        // TODO You will need to handle resizing this in parser.y
        // 给headers设置一个初始容量，如果初始容量还不够大，则在parser.y里翻倍的扩容
		request->current_request->header_capacity = default_header_capacity;
		request->current_request->headers = (Request_header *) malloc(sizeof(Request_header)*request->current_request->header_capacity);
		set_parsing_options(buf, i, request->current_request);

		if (yyparse() == SUCCESS) {
            //return request;
			if(buffer[i] != '\0' && buffer[i] != '\r' && buffer[i] != '\n'){
				request->next_request = (struct Requests *)chunked_parse(buffer + i, size);
			}
			return request;
		}
		else{
			request->current_request = NULL;
			request->next_request = NULL;
			yyrestart(yyin);
			printf("Parsing Failed: file input reset\n");
			if(buffer[i] != '\0' && buffer[i] != '\r' && buffer[i] != '\n'){
				request->next_request = (struct Requests *)chunked_parse(buffer + i, size);
			}
			
			//yyrestart(yyin); //输入文件重置
			//free(request->current_request->headers);
			//free(request);
			//printf("Parsing Failed: file input reset\n");
			//return NULL;
			return request;
		}
	}
	else{
		return NULL;
	} 

}
