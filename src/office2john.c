/* office2john.c: written by Dhiru Kholia in Summer of 2012 and released
 * under GNU LGPL. Based on test-dump-msole.c (under LPGL) and OoXmlCrypto.cs
 * (under LGPL).
 *
 * test-dump-msole.c: Export a msole file to a directory tree
 *
 * Copyright (C) 2002-2006	Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 *
 * *****************************************************************************
 *  _                      _     _ _ _
 * | |   _   _  __ _ _   _(_) __| (_) |_ _   _
 * | |  | | | |/ _` | | | | |/ _` | | __| | | |
 * | |__| |_| | (_| | |_| | | (_| | | |_| |_| |
 * |_____\__, |\__, |\__,_|_|\__,_|_|\__|\__, |
 *       |___/    |_|                    |___/
 *
 *  Decrypytion library for Office Open XML files
 *  API Version: 2008-12-12
 *  Generated: Wed Dec 12 12:33:00 GMT 2008
 *
 * *****************************************************************************
 *  Copyright Lyquidity Solutions Limited 2008
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *  You may not use this file except in compliance with permission LGPL2 license.
 *  You may obtain a copy of the License at:
 *
 *  http://creativecommons.org/licenses/by-sa/3.0/
 *
 *  This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the
 *  specific language governing permissions and limitations under the License.
 * *****************************************************************************
 *
 * Algorithms in this code file are based on the MS-OFFCRYPT.PDF provided by
 * Microsoft as part of its Open Specification Promise (OSP) program and which
 * is available here:
 *
 * http://msdn.microsoft.com/en-us/library/cc313071.aspx */

#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-msole.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-stdio.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

static void print_hex(const unsigned char *str, int len)
{
	int i;
	for (i = 0; i < len; ++i)
		printf("%02x", str[i]);
}

enum AlgId {
	ByFlags = 0x00,
	AIRC4 = 0x00006801,
	AES192 = 0x0000660F,
	AES256 = 0x00006610
};

enum AlgHashId {
	AHIAny = 0x00,
	AHIRC4 = 0x00008000,
	SHA1 = 0x00008004
};

enum ProviderType {
	PTAny = 0x00000000,
	PTRC4 = 0x00000001,
	AES = 0x00000018
};

enum EncryptionFlags {
	None = 0,
	Reserved1 = 1,		// MUST be 0, and MUST be ignored.
	Reserved2 = 2,		// MUST be 0, and MUST be ignored.
	fCryptoAPI = 4,		// A flag that specifies whether CryptoAPI RC4 or [ECMA-376] encryption is used. MUST be 1 unless fExternal is 1. If fExternal is 1, MUST be 0.
	fDocProps = 8,		// MUST be 0 if document properties are encrypted. Encryption of document properties is specified in section 2.3.5.4.
	fExternal = 16,		// If extensible encryption is used, MUST be 1. If this field is 1, all other fields in this structure MUST be 0.
	fAES = 32,		// If the protected content is an [ECMA-376] document, MUST be 1. If the fAES bit is 1, the fCryptoAPI bit MUST also be 1.
	fAgile = 0x40,
};

static uint32_t fget32(FILE * fp)
{
	uint32_t v = fgetc(fp);
	v |= fgetc(fp) << 8;
	v |= fgetc(fp) << 16;
	v |= fgetc(fp) << 24;
	return v;
}

static uint16_t fget16(FILE * fp)
{
	uint32_t v = fgetc(fp);
	v |= fgetc(fp) << 8;
	return v;
}


static void process_file(char *filename, char *parentfile)
{
	FILE *fp;
	int i;
	struct stat sb;

	if(stat(filename, &sb) != 0) {
	}

	if (!(fp = fopen(filename, "rb"))) {
		fprintf(stderr, "! %s : %s\n", filename, strerror(errno));
		return;
	}

	while (!feof(fp)) {
		uint16_t versionMajor = fget16(fp);
		uint16_t versionMinor = fget16(fp);
		uint32_t encryptionFlags = fget32(fp);
		if (encryptionFlags == fExternal) {
			fprintf(stderr, "%s : An external cryptographic provider is not supported\n", parentfile);
			return;
		}
		if (versionMinor == 0x04 && versionMajor == 0x04) { /* Office 2010 files */
			if (encryptionFlags != fAgile)
				fprintf(stderr, "%s : The encryption flags are not consistent with the encryption type\n", parentfile);
			/* rest of the data is in XML format, dump it to a file */
			char path[4096] = { 0 };
			strcpy(path, filename);
			char *buffer = (char*)malloc(sb.st_size);
			fread(buffer, sb.st_size - 8, 1, fp);
			char *xmlfile = strcat(path, ".xml");
			FILE *ofp;
			if (!(ofp = fopen(xmlfile, "w"))) {
				fprintf(stderr, "! %s : %s\n", filename, strerror(errno));
				return;
			}
			fwrite(buffer, sb.st_size - 8, 1, ofp);
			fclose(ofp);
			/* process XML file */
			xmlDocPtr doc;
			xmlNodePtr cur;
			doc = xmlParseFile(xmlfile);
			if (doc == NULL ) {
				fprintf(stderr, "Document not parsed successfully. \n");
				return;
			}
			cur = xmlDocGetRootElement(doc);
			if (cur == NULL) {
				fprintf(stderr, "empty document\n");
				xmlFreeDoc(doc);
				return;
			}
			cur = cur->xmlChildrenNode;
			while (cur != NULL) {
				if ((!xmlStrcmp(cur->name, (const xmlChar *)"keyEncryptors")))
					break;
				cur = cur->next;
			}
			cur = cur->xmlChildrenNode;
			while (cur != NULL) {
				if ((!xmlStrcmp(cur->name, (const xmlChar *)"keyEncryptor")))
					break;
				cur = cur->next;
			}
			cur = cur->xmlChildrenNode;
			/* we are now at "encryptedKey" node */
			xmlChar *spinCountXML = xmlGetProp(cur, "spinCount");
			int spinCount = atoi(spinCountXML);
			xmlFree(spinCountXML);
			xmlChar *saltSizeXML = xmlGetProp(cur, "saltSize");
			int saltSize = atoi(saltSizeXML);
			xmlFree(saltSizeXML);
			xmlChar *pkeBlockSizeXML = xmlGetProp(cur, "blockSize");
			int pkeBlockSize = atoi(pkeBlockSizeXML);
			xmlFree(pkeBlockSizeXML);
			xmlChar *pkeKeyBitsXML = xmlGetProp(cur, "keyBits");
			int pkeKeyBits = atoi(pkeKeyBitsXML);
			xmlFree(pkeKeyBitsXML);
			xmlChar *pkeHashSizeXML = xmlGetProp(cur, "hashSize");
			int pkeHashSize = atoi(pkeHashSizeXML);
			xmlFree(pkeHashSizeXML);
			xmlChar *pkeSaltValueXML = xmlGetProp(cur, "saltValue");
			unsigned char *pkeSaltValue = (unsigned char*)malloc(16 + 2);
			base64_decode(pkeSaltValueXML, strlen(pkeSaltValueXML), pkeSaltValue);
			xmlFree(pkeSaltValueXML);
			xmlChar *encryptedVerifierHashInputXML = xmlGetProp(cur, "encryptedVerifierHashInput");
			unsigned char encryptedVerifierHashInput[16 + 2];
			base64_decode(encryptedVerifierHashInputXML, strlen(encryptedVerifierHashInputXML), encryptedVerifierHashInput);
			xmlFree(encryptedVerifierHashInputXML);
			xmlChar *encryptedVerifierHashValueXML = xmlGetProp(cur, "encryptedVerifierHashValue");
			unsigned char encryptedVerifierHashValue[32 + 2];
			base64_decode(encryptedVerifierHashValueXML, strlen(encryptedVerifierHashValueXML), encryptedVerifierHashValue);
			xmlFree(encryptedVerifierHashValueXML);
			int version = 2010;
			printf("%s:$office$*%d*%d*%d*%d*", parentfile, version, spinCount, pkeKeyBits, saltSize);
			print_hex(pkeSaltValue, saltSize);
			printf("*");
			print_hex(encryptedVerifierHashInput, 16);
			printf("*");
			print_hex(encryptedVerifierHashValue, 32);
			printf("\n");
			xmlFreeDoc(doc);
			unlink(xmlfile);
			return;
		}
		/* Office 2007 file processing */
		// Encryption header
		uint32_t headerLength = fget32(fp);
		uint32_t skipFlags = fget32(fp);
		headerLength -= 4;
		uint32_t sizeExtra = fget32(fp);
		headerLength -= 4;
		uint32_t algId = fget32(fp);
		headerLength -= 4;
		uint32_t algHashId = fget32(fp);
		headerLength -= 4;
		uint32_t keySize = fget32(fp);
		headerLength -= 4;
		uint32_t providerType = fget32(fp);
		headerLength -= 4;
		(void)fget32(fp);
		headerLength -= 4;	// Reserved 1
		(void)fget32(fp);
		headerLength -= 4;	// Reserved 2
		char CSPName[1024];
		fread(CSPName, headerLength, 1, fp);
		// Encryption verifier
		uint32_t saltSize = fget32(fp);
		char salt[1024];
		fread(salt, saltSize, 1, fp);
		char encryptedVerifier[16];
		fread(encryptedVerifier, 16, 1, fp);
		uint32_t verifierHashSize = fget32(fp);
		char encryptedVerifierHash[64];
		if(providerType == PTRC4)
			fread(encryptedVerifierHash, 0x14, 1, fp);
		else
			fread(encryptedVerifierHash, 0x20, 1, fp);
		int version = 2007;
		printf("%s:$office$*%d*%d*%d*%d*", parentfile, version, verifierHashSize, keySize, saltSize);
		print_hex(salt, saltSize);
		printf("*");
		print_hex(encryptedVerifier, 16);
		printf("*");
		print_hex(encryptedVerifierHash, 32);
		printf("\n");
		break;
	}
}

static void clone(GsfInput * input, GsfOutput * output)
{
	guint8 const *data;
	size_t len;
	int i;

	if (gsf_input_size(input) > 0) {
		while ((len = gsf_input_remaining(input)) > 0) {
			/* copy in odd sized chunks to exercise system */
			if (len > 314)
				len = 314;
			if (NULL == (data = gsf_input_read(input, len, NULL))) {
				g_warning("error reading ?");
				return;
			}
			if (!gsf_output_write(output, len, data)) {
				g_warning("error writing ?");
				return;
			}
		}
	}

	gsf_output_close(output);
	g_object_unref(G_OBJECT(output));
	g_object_unref(G_OBJECT(input));
}

static int test(char *argv[])
{
	GsfInput *input;
	GsfInfile *infile;
	GsfOutfile *outfile;
	GError *err = NULL;

	input = gsf_input_stdio_new(argv[1], &err);
	if (input == NULL) {
		g_return_val_if_fail(err != NULL, 1);
		fprintf(stderr, "%s : No such file!\n", argv[1], err->message);
		g_error_free(err);
		return 1;
	}

	infile = gsf_infile_msole_new(input, &err);
	g_object_unref(G_OBJECT(input));

	if (infile == NULL) {
		g_return_val_if_fail(err != NULL, 1);
		fprintf(stderr, "%s : %s, maybe the file is not encrypted!\n", argv[1], err->message);
		g_error_free(err);
		return 1;
	}

	GsfInfile *in = GSF_INFILE(infile);
	GsfInput *src;
	GsfOutput *dst;
	src = gsf_infile_child_by_name(in, "EncryptionInfo");
	if (!src) {
		fprintf(stderr, "%s : is not a Office 2007 / 2010 encrypted file!\n", argv[1]);
		return 1;
	}

	char template[] = "office2johnXXXXXX";
	char *dirname = mktemp(template);
	if (!dirname) {
		perror("mkdtemp");
		exit(-1);
	}
	outfile = gsf_outfile_stdio_new(dirname, &err);
	if (outfile == NULL) {
		g_return_val_if_fail(err != NULL, 1);
		fprintf(stderr, "%s : %s\n", argv[1], err->message);
		g_error_free(err);
		return 1;
	}

	GsfOutfile *out = GSF_OUTFILE(outfile);

	dst = gsf_outfile_new_child(out, "EncryptionInfo", FALSE);
	clone(src, dst);

	char outpath[4096];
	sprintf(outpath, "%s/EncryptionInfo", dirname);

	process_file(outpath, argv[1]);

	int ret = unlink(outpath);
	ret = rmdir(dirname);
	return 0;
}

int main(int argc, char *argv[])
{
	int res;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <Office 2007 / 2010 encrypted files>\n", argv[0]);
		return 1;
	}

	gsf_init();
	res = test(argv);
	gsf_shutdown();

	return res;
}
