/**
 *============================================================
 *  @file      xmlutils.h
 *
 *  @brief    utilities for manipulating xml files
 * 
 *  compiler   gcc4.3.2
 *
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef XMLUTILS_H_
#define XMLUTILS_H_

#include <cstring>
#include <vector>
#include <typeinfo>
#include <sstream>

#include "utils.h"

extern "C" {
#include <libxml/tree.h>
#include <libtaomee/log.h>
}


/* @brief 得到prop对应的内容
 * @param xml对应的节点
 * @param prop 字段名称 
 * @param str 保存prop字段对应的内容
 * @note 调用此函数，需手动释放str对应的内存
 */
#define DECODE_XML_PROP(cur, prop, str) \
		do { \
			str = xmlGetProp(cur, (xmlChar*)prop);  \
			if (!str) { \
                ERROR_LOG("xml parse error:[node:%s prop=%s]", cur->name, prop); \
				return -1; \
			} \
		} while (0)

/* @brief 得到prop字段对应的int值，为空取默认值
 * @param v 保存获取的数值
 * @param def 如果字段为空则取这默认值
 */
#define DECODE_XML_PROP_INT_DEFAULT(v, cur, prop, def) \
		do { \
			xmlChar *str; \
			if (!cur || !(str = xmlGetProp(cur, (xmlChar*)prop))) { \
				v = def; \
			} else { \
				v = atoi ((const char*)str); \
				xmlFree (str); \
			} \
		} while (0)

/* @brief 得到prop字段对应的int值，为空取默认值
 * @param v 保存获取的数值
 * @param def 如果字段为空则取这默认值
 */
#define DECODE_XML_PROP_INT_DEFAULT_HEX(v, cur, prop, def) \
		do { \
			xmlChar *str; \
			if (!cur || !(str = xmlGetProp(cur, (xmlChar*)prop))) { \
				v = def; \
			} else { \
				sscanf((char *)str, "%x", &v); \
				xmlFree (str); \
			} \
		} while (0)


/* @brief 得到prop对应的int值
 * @note 不提供默认数值，此字段数值必须存在
 */
#define DECODE_XML_PROP_INT(v, cur, prop) \
		do { \
			xmlChar *str; \
			DECODE_XML_PROP(cur, prop, str); \
			v = atoi ((const char*)str); \
			xmlFree (str); \
		} while (0)

/* @brief 得到prop字段的无符号整数uint32_t值
 * @note 不提供默认数值，此字段数值必须存在
 */
#define DECODE_XML_PROP_UINT32(v, cur, prop) \
		do { \
			xmlChar *str; \
			DECODE_XML_PROP(cur, prop, str); \
			char* endpt; \
			v = strtoul((const char*)str, &endpt, 10); \
			xmlFree (str); \
		} while (0)

/* @brief 得到prop字段对应的uint32值，为空取默认值
 * @param v 保存获取的数值
 * @param def 如果字段为空则取这默认值
 */
#define DECODE_XML_PROP_UINT32_DEFAULT(v, cur, prop, def) \
		do { \
			xmlChar *str; \
			if (!cur || !(str = xmlGetProp(cur, (xmlChar*)prop))) { \
				v = def; \
			} else { \
                char *endpt; \
				v = strtoul((const char*)str, &endpt, 10); \
				xmlFree (str); \
			} \
		} while (0)

/* @brief 解析prop字段包含的多个数字
 * @param arr_ 保存的数组地址
 * @param len_ 数组的长度
 * @param cur_ xml节点
 * @param prop_ 字段名称
 * @note  多个数字之间用空格分隔
 */
#define DECODE_XML_PROP_ARR_INT(arr_, len_, cur_, prop_) \
		do { \
			xmlChar* str; \
			DECODE_XML_PROP((cur_), (prop_), str); \
			int i = 0, cnt = 0, k; \
			size_t slen = strlen((const char*)str); \
			for (; (i != (len_)) && (cnt != slen); ++i, cnt += k) { \
				sscanf((const char*)str + cnt, "%d%n", &((arr_)[i]), &k); \
			} \
			xmlFree(str); \
		} while (0)


/* @brief 解析prop字段包含的多个数字
 * @param arr_ 保存的数组地址
 * @param len_ 数组的长度
 * @param cur_ xml节点
 * @param prop_ 字段名称
 * @note  多个数字之间用空格分隔
 */
#define DECODE_XML_PROP_ARR_UINT32(arr_, len_, cur_, prop_) \
		do { \
			xmlChar* str; \
			DECODE_XML_PROP((cur_), (prop_), str); \
			int i = 0, cnt = 0, k; \
			size_t slen = strlen((const char*)str); \
			for (; (i != (len_)) && (cnt != slen); ++i, cnt += k) { \
				sscanf((const char*)str + cnt, "%ul%n", &((arr_)[i]), &k); \
			} \
			xmlFree(str); \
		} while (0)


/* @brief 得到prop字段对应的字符串的内容
 */
#define DECODE_XML_PROP_STR(v, cur, prop) \
		do { \
			xmlChar* str; \
			DECODE_XML_PROP(cur, prop, str); \
            STRCPY_SAFE(v, (const char *)str); \
			v[sizeof(v) - 1] = '\0'; \
			xmlFree(str); \
		} while (0)

/* @brief 得到prop字段对应的字符串的内容
 */
#define DECODE_XML_PROP_STR_DEFAULT(v, cur, prop, def) \
		do { \
			xmlChar* str; \
			str = xmlGetProp(cur, (xmlChar*)prop);  \
			bool need_free = true;\
			if (!str) { \
				str = (xmlChar *)def;\
				need_free = false;\
			} \
            STRCPY_SAFE(v, (const char*)str); \
			v[sizeof(v) - 1] = '\0'; \
			if (need_free) { \
				xmlFree(str); \
			}\
		} while (0)

/* @brief 解析prorp字段对应的数值
 */
static inline void decode_xml_prop_uint32_default(uint32_t* val, xmlNodePtr cur, const void* prop, uint32_t def)
{
	xmlChar* str;
	if (!cur || !(str = xmlGetProp(cur, (const xmlChar *)(prop)))) {
		*val = def;
	} else {
		char* endpt;
		*val = strtoul((char*)(str), &endpt, 10);
		xmlFree(str);
	}
}

/* @brief 解析prop字段对应的float数值
 * 
 */
static inline void decode_xml_prop_float_default(float* val, xmlNodePtr cur, const void* prop, float def)
{
	xmlChar* str;
	if (!cur || !(str = xmlGetProp(cur, (const xmlChar *)(prop)))) {
		*val = def;
	} else {
		sscanf((char *)(str), "%f", val);
		xmlFree(str);
	}
}

/* @brief 加载配置文件
 * @param file 加载的配置文件名称
 * @param parser 对配置文件进行解析的函数指针
 * @note file名称中包含路径
 */
static inline int load_xmlconf(const char* file, int (*parser)(xmlNodePtr cur_node))
{
	int err = -1;

	xmlDocPtr doc = xmlParseFile(file);
	if (!doc) {
        xmlErrorPtr xptr = xmlGetLastError();
		ERROR_RETURN(("Failed to Load %s [line:%u msg:%s]", file, xptr->line, xptr->message), -1);
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc); 
	if (!cur) {
		ERROR_LOG("xmlDocGetRootElement error when loading file '%s'", file);
		goto fail;
	}

	err = parser(cur);
fail:
	xmlFreeDoc(doc);
	RT_SCREEN_TLOG(err, "Load File %s", file);
}

/* @brief 重新加载配置文件
 * @param file 加载的文件名
 * @param parser 实现解析的函数指针
 */
static inline void reload_xmlconf(const char* file, int (*parser)(xmlNodePtr cur_node))
{
	xmlDocPtr doc = xmlParseFile(file);
	if (!doc) {
		KINFO_LOG(0, "Failed to Reload '%s'", file);
		return;
	}

	int err = -1;
	xmlNodePtr cur = xmlDocGetRootElement(doc); 
	if (!cur) {
		KINFO_LOG(0, "xmlDocGetRootElement error when reloading file '%s'", file);
		goto fail;
	}

	err = parser(cur);
fail:
	xmlFreeDoc(doc);
	KINFO_LOG(0, "Reload File '%s' %s", file, (err ? "Failed" : "Succeeded"));
}



/* @brief 读取数组内容
 * @param arr 保存prop字段对应的内容的字符串
 * @param len arr字符串长度
 * @param cur xml对应的节点
 * @param prop 字段名称
 * @param def 读取不成功，arr的默认值
 */
template <typename T1, typename T2, size_t len>
size_t decode_xml_prop_arr_default(T1 (&arr)[len], xmlNodePtr cur, const void* prop, const T2& def)
{
	xmlChar* str;
	if (!cur || !(str = xmlGetProp(cur, (xmlChar*)prop))) {
		for (size_t i = 0; i != len; ++i) {
			arr[i] = def;
		}
		return 0;
	} else {
		size_t i = 0;
		std::istringstream iss(reinterpret_cast<const char*>(str));
		while ((i != len) && (iss >> arr[i])) {
			if ((typeid(T1) == typeid(uint8_t)) || (typeid(T1) == typeid(int8_t))) {
				arr[i] -= '0';
			}
			++i;
		}

		xmlFree(str);
		return i;
	}
}


/* @brief 读取字符串内容到vector
 * @param vct 保存prop字段对应的内容的字符串的
 * @param cur xml对应的节点
 * @param prop 字段名称
 */
template <typename T>
size_t decode_xml_prop_vct(std::vector<T>& vct, xmlNodePtr cur, const void* prop)
{
	xmlChar* str;
	if (!cur || !(str = xmlGetProp(cur, (xmlChar*)prop))) {
		return 0;
	}

	T temp;
	std::istringstream iss(reinterpret_cast<const char*>(str));
	while (!iss.eof() && (iss >> temp)) {
		if ((typeid(T) == typeid(uint8_t)) || (typeid(T) == typeid(int8_t))) {
			temp -= '0';
		}
		if (temp) {
			vct.push_back(temp);
		}
	}

	xmlFree(str);
	return vct.size();
}

template <typename T1, typename T2>
void decode_xml_prop_default(T1& val, xmlNodePtr cur, const void* prop, const T2& def)
{
	xmlChar* str;
	if (!cur || !(str = xmlGetProp(cur, reinterpret_cast<const xmlChar*>(prop)))) {
		val = def;
	} else {
		std::istringstream iss(reinterpret_cast<const char*>(str));
		iss >> val;
		xmlFree(str);
	}
}


#endif
