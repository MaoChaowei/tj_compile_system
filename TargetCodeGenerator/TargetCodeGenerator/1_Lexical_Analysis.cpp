#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <vector>
#include "1_Lexical_Analysis.h"
using namespace std;

vector<TOKEN>lexAnswer;  // 1、词法分析器的结果

// path为C语言源程序文件路径
void lexAnalysis(const string path);  // 调用接口，调用本函数进行词法分析

void init();  // 加载保留字
//string preProcessing(string src);   // 源程序预处理
int findReservedWord(string word);  // 查找保留字并返回其编码，未找到返回-1
void scanner(string dst, int& pointer, ofstream& outfile);   // 扫描器
void readRealNumber(string src, int& pointer, string& dst, int& col);  // 数字读取

// 保留字
string reservedWord[] = { "(", ")", "{",  "}", ",", ";", "\"",
						  "=", "+", "-", "*", "/", "+=", "-=", "*=", "/=", "%=", "^=", "&=", "|=",
						  ">", "<", ">=", "<=", "==", "!=", "||", "&&",
						  "else", "if", "int", "float", "double", "return", "void", "while" };

map<string, int>wordMap;   // 保留字与其编码的哈希关联

void lexAnalysis(const string path) {
	// --------------------
	// 读取源程序 
	// --------------------

	ifstream infile;
	ofstream outfile;
	infile.open(path, ios::in);
	if (!infile.is_open()) {
		cout << "文件打开失败~" << endl;
		exit(-1);
	}
	string src;
	//将文件内容全部写入字符串中
	src.assign(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

	// ------------------------
	// 扫描输出单词的二元组列表
	// ------------------------

	init();
	outfile.open("../outfiles/1_Lexical_AnalysisAnswer.txt", ios::out | ios::trunc);
	if (!outfile.is_open()) {
		cout << "文件打开失败~" << endl;
		exit(-1);
	}
	int pointer = 0;  // 源程序指针
	while (pointer < src.length()) {
		scanner(src, pointer, outfile);
		if (!src[pointer]) break;
	}
	outfile.close();
}

// 加载保留字
void init() {
	int i = 0;
	for (auto word : reservedWord) {
		wordMap.insert(pair<string, int>(word, ++i));
	}
}


// 查找保留字并返回其编码，未找到返回-1
int findReservedWord(string word) {
	auto iter = wordMap.find(word);
	return iter == wordMap.end() ? -1 : wordMap[word];
}

// 扫描器，核心算法
// 输出单词种别及其属性值的二元组
void scanner(string dst, int& pointer, ofstream& outfile) {
	static int row = 1;
	static int col = 1;
	static bool lastIsIdentifier = false; // 上一词是否为标识符，用于判断读入实常数时的“-”是负号还是减号

	int coding = -1;   // 单词编码工作变量
	string word = "";  // 单词工作变量
	string attribute = "";  // 属性工作变量
	//preprocessing
	do {
		while (dst[pointer] == ' ' || dst[pointer] == '\r' || dst[pointer] == '\n') {
			if (dst[pointer] == ' ') {
				pointer++;
				col++;
			}
			else {
				row++;
				col = 1;
				pointer++;
			}
			if (pointer >= dst.size())
				return;
		}
		// 删除行注释
		if (dst[pointer] == '/' && ((pointer + 1) < dst.length()) && dst[pointer + 1] == '/') {
			for (; pointer < dst.length() && dst[pointer] != '\r' && dst[pointer] != '\n'; pointer++)
				col++;
			if (pointer >= dst.length())//末尾则返回
				return;
			else {//换行则修改row col
				row++;
				col = 1;
				pointer++;
			}
		}
		if (dst[pointer] == '/' && ((pointer + 1) < dst.length()) && dst[pointer + 1] == '*') {
			for (; pointer < dst.length() && !(dst[pointer - 1] == '*' && dst[pointer] == '/'); pointer++) {
				if (dst[pointer] == '\r' || dst[pointer] == '\n') {
					row++;
					col = 1;
				}
				else
					col++;
			}
			if (pointer == dst.length()) {
				cerr << "源程序错误！没有找到\"*/\"!";
			}
			pointer++;
		}
	} while ((dst[pointer] == ' ' || dst[pointer] == '\r' || dst[pointer] == '\n')
		|| (dst[pointer] == '/' && ((pointer + 1) < dst.length()) && dst[pointer + 1] == '/')
		|| (dst[pointer] == '/' && ((pointer + 1) < dst.length()) && dst[pointer + 1] == '*'));


	if (isalpha(dst[pointer]) || isdigit(dst[pointer]) || dst[pointer] == '_') {  // 数字、字母、下划线 开头

		if (isalpha(dst[pointer]) || dst[pointer] == '_') {   // 字母、下划线 开头

			while (isalpha(dst[pointer]) || isdigit(dst[pointer]) || dst[pointer] == '_') {
				word += dst[pointer++];  //收集，然后下移
				col++;
			}

			coding = findReservedWord(word);   // 查表，若是保留字，返回种别码；

			attribute = coding == -1 ? word : "--";
			word = coding == -1 ? "identifier" : word;
		}
		else {   //数字开头（识别实常数）
			word = "number";
			readRealNumber(dst, pointer, attribute, col);
		}
	}
	else if (dst[pointer] == '\"') {  // 字符串常量
		pointer++;
		col++;
		word += '\"';
		while (dst[pointer] != '\"') {
			word += dst[pointer];
			pointer++;
			col++;
		}
		word += '\"';
		attribute = word;
		word = "const string";
		pointer++;
		col++;
	}
	else {  // 开头为其它字符
		word = dst[pointer];
		word += dst[pointer + 1];   // 超前搜索
		if ((coding = findReservedWord(word)) != -1) {  // || != <= == >=
			pointer += 2;
			col += 2;
			attribute = "--";
		}
		else if (!lastIsIdentifier && (word[0] == '-' || word[0] == '.') && isdigit(word[1]))	// 说明此处“-”为负号
		{
			word = "number";
			readRealNumber(dst, pointer, attribute, col);
		}
		else {
			word = dst[pointer];
			if ((coding = findReservedWord(word)) != -1) {  // ( ) * + , - / ; < >
				pointer += 1;
				col += 1;
			}
			else
			{
				cout << "ERROR:" << endl;
				exit(EXIT_FAILURE);
			} // 错误处理-------------------------------------------------------------
			attribute = "--";
		}
	}
	lastIsIdentifier = (attribute == "--") ? false : true;
	outfile << "< " << word << " , " << attribute << " >" << " ROW: " << row << " COL: " << col << endl;

	TOKEN token = { word,attribute,row,col };//***********************************TODO:实现行列计数
	lexAnswer.push_back(token);
}

// 读取实常数的函数
// 实常数的类型：
// 十进制数 / 八进制数：0开头 / 十六进制数：0x开头
// 小数：有且仅有一个小数点
// 指数：有且仅有一个e，且e前必须有数，e后必须整数
// 注：若为小数或指数，则前导0无效；若为整数，则前导0指示八进制
// 注：小数点后可以有一个e，但e后不能有小数点
// 注：若0x前导十六进制，则负号只能在0x前面
void readRealNumber(string src, int& pointer, string& dst, int& col)
{
	dst = "";
	if (src[pointer] == '-') {
		dst += src[pointer++];
		col++;
	}

	bool is_decimal = true;
	if (src[pointer] == '0')
	{
		dst += src[pointer++];
		col++;
		if (src[pointer] == 'x' || src[pointer] == 'X') // 16进制
		{
			dst += src[pointer++];
			while (isdigit(src[pointer]) || (src[pointer] >= 'a' && src[pointer] <= 'f')
				|| (src[pointer] >= 'A' && src[pointer] <= 'F')) {
				dst += src[pointer++];
				col++;
			}
			is_decimal = false;
		}
		else // 无法确定是十进制还是八进制，因此使用tmp_p
		{
			// 当数字在0-7中，对于两种进制都是合法的输入
			while (src[pointer] >= '0' && src[pointer] <= '7') {
				dst += src[pointer++];
				col++;
			}

			// 当数字超出0-7的范围，此时不确定这串数字是否是十进制，因此暂存
			int tmp_p = pointer;
			string dst_tmp = dst;
			while (isdigit(src[tmp_p])) {
				dst_tmp += src[tmp_p++];
			}

			// 只有遇到e或.，才能确定是十进制，否则就是八进制
			if (src[tmp_p] == 'e' || src[tmp_p] == 'E' || src[tmp_p] == '.')
			{
				is_decimal = true;
				dst = dst_tmp;
				pointer = tmp_p;
			}
			else
				is_decimal = false;
		}
	}

	if (is_decimal)
	{
		while (isdigit(src[pointer])) {
			dst += src[pointer++];
			col++;
		}
		if (src[pointer] == 'e' || src[pointer] == 'E')
		{
			dst += src[pointer++];
			col++;
			if (src[pointer] == '-') {
				dst += src[pointer++];
				col++;
			}
			while (isdigit(src[pointer])) {
				dst += src[pointer++];
				col++;
			}
		}
		else if (src[pointer] == '.')
		{
			dst += src[pointer++];
			col++;
			while (isdigit(src[pointer])) {
				dst += src[pointer++];
				col++;
			}
			if (src[pointer] == 'e' || src[pointer] == 'E')
			{
				dst += src[pointer++];
				col++;
				if (src[pointer] == '-') {
					dst += src[pointer++];
					col++;
				}
				while (isdigit(src[pointer])) {
					dst += src[pointer++];
					col++;
				}
			}
		}
	}
}

/*
源程序预处理
多个空格、回车、制表符均变为一个空格
注释忽略:行注释 段注释
*/
//string preProcessing(string src)
//{
//	int pointer = 0;//字符串指针，指向当前dst的末尾位置
//	int length = src.length();
//	string dst;
//
//	for (int i = 0; i < length; i++) {
//		//处理字符串常量
//		if (src[i] == '"') {
//			do {
//				pointer++;
//				dst += src[i++];
//			} while (src[i] != '"');
//			pointer++;
//			dst += src[i];
//		}
//		//回车换行制表 变为一个空格
//		else if (src[i] == ' ' || src[i] == '\r' || src[i] == '\n' || src[i] == '\t') {
//			//前一个写入dst的字符不是空格则能加入空格
//			if (pointer && dst[pointer - 1] != ' ') {
//				pointer++;
//				dst += ' ';
//			}
//		}
//		//注释删除
//		else if (src[i] == '/') {
//			if (i + 1 == length) {	//最后一个token是单独的“/”
//				pointer++;
//				dst += '/';
//				break;
//			}
//			if (src[i + 1] == '/') {//行注释
//				for (; i < length && src[i] != '\r' && src[i] != '\n'; i++)
//					;
//			}
//			else if (src[i + 1] == '*') {//段注释
//				for (; i < length && !(src[i - 1] == '*' && src[i] == '/'); i++)
//					;
//				if (i == length) {
//					cerr << "源程序错误！没有找到\"*/\"!";
//				}
//			}
//			else {
//				pointer++;
//				dst += '/';
//			}
//		}
//		else {
//			dst += src[i];
//			pointer++;
//		}
//	}
//	return dst;
//}