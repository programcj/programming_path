
//Content-Disposition: form-data; name="upgrade_file"; filename="httpAPI.txt"
void http_content_get(const char* ptr, const char* name, char* value, int vlen)
{
	while (*ptr)
	{
		if (*ptr == '\r' && *(ptr + 1) == '\n')
			break;
		if (*ptr == ';')
		{
			ptr++;
			if (*ptr == ' ')
				ptr++;
			if (strncmp(ptr, name, strlen(name)) == 0)
			{
				ptr += strlen(name);
				if (*ptr == '=')
				{
					ptr++;
					if (*ptr == '\"')
						ptr++;
					while (*ptr && vlen > 0)
					{
						if (*ptr == '\"' || *ptr == ';' || *ptr == '\r'
							|| *ptr == '\n')
							break;
						vlen--;
						*value++ = *ptr++;
					}
				}
			}
		}
		ptr++;
	}
}

//multipart/form-data 查找结束位置
const char* http_from_data_end(const char* ptr, int len, const char* boundary)
{
	const char* pend = ptr + len;
	//\r\n--
	while (ptr < pend)
	{
		if (memcmp(ptr, "\r\n--", 4) == 0 && strncmp(ptr + 4, boundary, strlen(boundary)) == 0)
			return ptr;

		if (memcmp(ptr, "\n--", 3) == 0 && strncmp(ptr + 3, boundary, strlen(boundary)) == 0)
			return ptr;

		if (strncmp(ptr, boundary, strlen(boundary)) == 0)
			return ptr;
		ptr++;
	}
	return NULL;
}



int from_data_boundary_cmp(const char* ptr, const char* boundary, int* end_index)
{
	int boundary_len = strlen(boundary);
	if (memcmp(ptr, "--", 2) == 0 && strncmp(ptr + 2, boundary, boundary_len) == 0)
	{
		if (memcmp(ptr + 2 + boundary_len, "\r\n", 2) == 0)
		{
			if (end_index)	*end_index = 2 + boundary_len + 2;
			return 0;
		}
		if (ptr[2 + boundary_len] == '\n')
		{
			if (end_index) *end_index = 2 + boundary_len + 1;
			return 0;
		}

		if (memcmp(ptr + 2 + boundary_len, "--", 2) == 0) //完全结束
		{
			if (end_index) *end_index = 2 + boundary_len + 2;
			return 0;
		}
	}
	return -1;
}

int form_data_boundary_find_next(const char* str, const char* end, const char* boundary)
{
	const char* ptr = str;
	while (ptr < end)
	{
		if (from_data_boundary_cmp(ptr, boundary, NULL) == 0) {
			return ptr - str;
		}
		ptr++;
	}
	return -1;
}

const char *http_header_get(const char* header, const char* name)
{
	const char* ptr=header;
	while (*ptr && memcmp(ptr, "\r\n\r\n", 4) != 0 && memcmp(ptr, "\n\n", 2) != 0)
	{
		if (strncmp(ptr, name, strlen(name)) == 0)
			return ptr;
		ptr++;
	}
	return NULL;
}

void http_from_data_parse(const char * boundary, void* bodyptr, int size,
	 void (*func_form_data)(void *ctx, const char *dispositionstr, void *bodystr, int size),
	 void *ctx)
{
/**************************************************************************************************
	* Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryAOugFE3GKIBKTVsI

------WebKitFormBoundaryAOugFE3GKIBKTVsI
Content-Disposition: form-data; name="file"; filename="facetest.jpg"
Content-Type: image/jpeg

**********
------WebKitFormBoundaryAOugFE3GKIBKTVsI
Content-Disposition: form-data; name="identifier"

lxphoto_diff
------WebKitFormBoundaryAOugFE3GKIBKTVsI
Content-Disposition: form-data; name="file"; filename="20210720140741.mp4_20210721_101725.021.jpg"
Content-Type: image/jpeg

***********
------WebKitFormBoundaryAOugFE3GKIBKTVsI--
************************************************************************************************************/
	int i = 0;
	const char* ptr = (const char*)bodyptr;
	const char* end = ptr + size;
	size_t boundary_len = strlen(boundary);
	//const char* form_data_name_ptr = NULL;
	//const char* content_type_ptr = NULL;
	int _endindex = 0;
	int ret;

	char form_data_name[100];
	char content_type[50];

	while(ptr<end)
	{
		_endindex = 0;
		if (from_data_boundary_cmp(ptr, boundary, &_endindex) == 0)
		{
			ptr += _endindex;
			if (ptr < end-1)
			{
				//这是一个
				memset(form_data_name, 0, sizeof(form_data_name));
				memset(content_type, 0, sizeof(content_type));

				const char *form_data_name_ptr= http_header_get(ptr, "Content-Disposition:");
				
				if (form_data_name_ptr) {
					form_data_name_ptr += strlen("Content-Disposition:");
					if (*form_data_name_ptr == ' ')
						form_data_name_ptr++;
					const char* eptr = form_data_name_ptr;
					while (*eptr != '\n' && *eptr != '\r')
						eptr++;

					int hlen = eptr - form_data_name_ptr;

					strncpy(form_data_name, form_data_name_ptr, hlen > sizeof(form_data_name) ? sizeof(form_data_name) : hlen);
				}
				const char *content_type_ptr = http_header_get(ptr, "Content-Type:");
				if (content_type_ptr)
				{
					content_type_ptr += strlen("Content-Type:");
					if (*content_type_ptr == ' ')
						content_type_ptr++;
					const char* eptr = content_type_ptr;
					while (*eptr != '\n' && *eptr != '\r')
						eptr++;

					int hlen = eptr - form_data_name_ptr;
					strncpy(content_type, content_type_ptr, hlen > sizeof(content_type) ? sizeof(content_type) : hlen);
				}

				while (ptr < end) {
					if (memcmp(ptr, "\r\n\r\n", 4) == 0) {
						ptr += 4;
						break;
					}
						
					if (memcmp(ptr, "\n\n", 2) == 0)
					{
						ptr += 2;
						break;
					}
					ptr++;
				}
				if (ptr < end)
				{
					ret = form_data_boundary_find_next(ptr, end, boundary);
					if (ret != -1)
					{
						if (*(ptr - 1) == '\n' && *(ptr - 2) == '\r')
							ret -= 2;

						//form-data; name="file"; filename="car1.jpg"
						char namestr[20];
						memset(namestr, 0, sizeof(namestr));
						http_content_get(form_data_name, "name", namestr, sizeof(namestr));

						//printf(">>>form-data: name:[%s],type:[%s] context-size:%d\n", namestr, content_type, ret);
						if (func_form_data)
							func_form_data(ctx, form_data_name, ptr, ret);
						ptr += ret;
						continue;
					}
				}
				
			}
		}		
		ptr++;
	}
}

int evhttp_request_get_content_type_boundary(struct evhttp_request* req, char * boundary, int size)
{
	struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
	const char* contentType = evhttp_find_header(headers, "Content-Type");
	const char* ptr = contentType ? strchr(contentType, '=') : NULL;
	//Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryKkFALQGN496lcMlO

	ptr = contentType ? strchr(contentType, '=') : NULL;
	if (ptr)
	{
		ptr++;
		strncpy(boundary, ptr, size);
	}
}

