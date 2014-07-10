/* apps/dhparam.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright (c) 1998-2000 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <openssl/opensslconf.h>	/* for OPENSSL_NO_DH */
#ifndef OPENSSL_NO_DH
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "apps.h"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#ifndef OPENSSL_NO_DSA
#include <openssl/dsa.h>
#endif


#define DEFBITS	512

static int dh_cb(int p, int n, BN_GENCB *cb);

const char* dhparam_help[] = {
	"-inform arg    input format, DER or PEM",
	"-outform arg   output format, DER or PEM",
	"-in arg        input file",
	"-out arg       output file",
	"-check         check the DH parameters",
	"-text          print a text form of the DH parameters",
	"-C             Output C code",
	"-2             generate parameters using  2 as the generator value",
	"-5             generate parameters using  5 as the generator value",
	"-rand file...  load the file(s) into the random number generator",
#ifndef OPENSSL_NO_DSA
	"-dsaparam      read or generate DSA parameters, convert to DH",
#endif
#ifndef OPENSSL_NO_ENGINE
	" -engine e     use engine e, possibly a hardware device.",
#endif
	NULL
};

enum options {
	OPT_ERR = -1, OPT_EOF = 0,
	OPT_INFORM, OPT_OUTFORM, OPT_IN, OPT_OUT,
	OPT_ENGINE, OPT_CHECK, OPT_TEXT, OPT_NOOUT,
	OPT_RAND, OPT_DSAPARAM, OPT_C, OPT_2, OPT_5,
};

static OPTIONS options[] = {
	{ "inform", OPT_INFORM, 'F' },
	{ "outform", OPT_OUTFORM, 'F' },
	{ "in", OPT_IN, '<' },
	{ "out", OPT_OUT, '>' },
	{ "check", OPT_CHECK, '-' },
	{ "text", OPT_TEXT, '-' },
	{ "noout", OPT_NOOUT, '-' },
	{ "rand", OPT_RAND, 's' },
	{ "C", OPT_C, '-' },
	{ "2", OPT_2, '-' },
	{ "5", OPT_5, '-' },
#ifndef OPENSSL_NO_ENGINE
	{ "engine", OPT_ENGINE, 's' },
#endif
#ifndef OPENSSL_NO_DSA
	{ "dsaparam", OPT_DSAPARAM, '-' },
#endif
	{ NULL }
};
int dhparam_main(int argc, char **argv)
	{
	DH *dh=NULL;
	int i,text=0;
	enum options o;
	BIO *in=NULL,*out=NULL;
	int informat=FORMAT_PEM,outformat=FORMAT_PEM,check=0,noout=0,C=0,ret=1;
	char *infile=NULL,*outfile=NULL,*prog;
	char *inrand=NULL;
	int num = 0, g = 0;
	int dsaparam=0;
	char *engine=NULL;

	prog = opt_init(argc, argv, options);
	while ((o = opt_next()) != OPT_EOF) {
		switch (o) {
		case OPT_EOF:
		case OPT_ERR:
			BIO_printf(bio_err,"Usage: %s [flags] [numbits]", prog);
			BIO_printf(bio_err,"Valid options are:\n");
			printhelp(dhparam_help);
			goto end;
		case OPT_INFORM:
			opt_format(opt_arg(), 1, &informat);
			break;
		case OPT_OUTFORM:
			opt_format(opt_arg(), 1, &outformat);
			break;
		case OPT_IN:
			infile = opt_arg();
			break;
		case OPT_OUT:
			outfile = opt_arg();
			break;
		case OPT_ENGINE:
			engine = opt_arg();
			break;
		case OPT_CHECK:
			check = 1;
			break;
		case OPT_TEXT:
			text = 1;
			break;
		case OPT_DSAPARAM:
			dsaparam = 1;
			break;
		case OPT_C:
			C=1;
			break;
		case OPT_2:
			g=2;
			break;
		case OPT_5:
			g=5;
			break;
		case OPT_NOOUT:
			noout=1;
			break;
		case OPT_RAND:
			inrand = opt_arg();
			break;
		}
	}

	argv = opt_rest();
	if (argv[0] && (!opt_int(argv[0], &num) || num <= 0))
		goto end;

#ifndef OPENSSL_NO_ENGINE
        setup_engine(bio_err, engine, 0);
#endif

	if (g && !num)
		num = DEFBITS;

#ifndef OPENSSL_NO_DSA
	if (dsaparam && g)
		{
		BIO_printf(bio_err, "generator may not be chosen for DSA parameters\n");
		goto end;
		}
#endif
	/* DH parameters */
	if (num && !g)
		g = 2;

	if(num) {

		BN_GENCB cb;
		BN_GENCB_set(&cb, dh_cb, bio_err);
		if (!app_RAND_load_file(NULL, bio_err, 1) && inrand == NULL)
			{
			BIO_printf(bio_err,"warning, not much extra random data, consider using the -rand option\n");
			}
		if (inrand != NULL)
			BIO_printf(bio_err,"%ld semi-random bytes loaded\n",
				app_RAND_load_files(inrand));

#ifndef OPENSSL_NO_DSA
		if (dsaparam)
			{
			DSA *dsa = DSA_new();
			
			BIO_printf(bio_err,"Generating DSA parameters, %d bit long prime\n",num);
			if(!dsa || !DSA_generate_parameters_ex(dsa, num,
						NULL, 0, NULL, NULL, &cb))
				{
				if(dsa) DSA_free(dsa);
				ERR_print_errors(bio_err);
				goto end;
				}

			dh = DSA_dup_DH(dsa);
			DSA_free(dsa);
			if (dh == NULL)
				{
				ERR_print_errors(bio_err);
				goto end;
				}
			}
		else
#endif
			{
			dh = DH_new();
			BIO_printf(bio_err,"Generating DH parameters, %d bit long safe prime, generator %d\n",num,g);
			BIO_printf(bio_err,"This is going to take a long time\n");
			if(!dh || !DH_generate_parameters_ex(dh, num, g, &cb))
				{
				ERR_print_errors(bio_err);
				goto end;
				}
			}

		app_RAND_write_file(NULL, bio_err);
	} else {

		in = bio_open_default(infile, RB(informat));
		if (in == NULL)
			goto end;

#ifndef OPENSSL_NO_DSA
		if (dsaparam)
			{
			DSA *dsa;
			
			if (informat == FORMAT_ASN1)
				dsa=d2i_DSAparams_bio(in,NULL);
			else /* informat == FORMAT_PEM */
				dsa=PEM_read_bio_DSAparams(in,NULL,NULL,NULL);
			
			if (dsa == NULL)
				{
				BIO_printf(bio_err,"unable to load DSA parameters\n");
				ERR_print_errors(bio_err);
				goto end;
				}
			
			dh = DSA_dup_DH(dsa);
			DSA_free(dsa);
			if (dh == NULL)
				{
				ERR_print_errors(bio_err);
				goto end;
				}
			}
		else
#endif
			{
			if (informat == FORMAT_ASN1)
				dh=d2i_DHparams_bio(in,NULL);
			else /* informat == FORMAT_PEM */
				dh=PEM_read_bio_DHparams(in,NULL,NULL,NULL);
			
			if (dh == NULL)
				{
				BIO_printf(bio_err,"unable to load DH parameters\n");
				ERR_print_errors(bio_err);
				goto end;
				}
			}
		
		/* dh != NULL */
	}
	
	out = bio_open_default(outfile, "w");
	if (out == NULL)
		goto end;

	if (text)
		{
		DHparams_print(out,dh);
		}
	
	if (check)
		{
		if (!DH_check(dh,&i))
			{
			ERR_print_errors(bio_err);
			goto end;
			}
		if (i & DH_CHECK_P_NOT_PRIME)
			printf("p value is not prime\n");
		if (i & DH_CHECK_P_NOT_SAFE_PRIME)
			printf("p value is not a safe prime\n");
		if (i & DH_UNABLE_TO_CHECK_GENERATOR)
			printf("unable to check the generator value\n");
		if (i & DH_NOT_SUITABLE_GENERATOR)
			printf("the g value is not a generator\n");
		if (i == 0)
			printf("DH parameters appear to be ok.\n");
		}
	if (C)
		{
		unsigned char *data;
		int len,l,bits;

		len=BN_num_bytes(dh->p);
		bits=BN_num_bits(dh->p);
		data=(unsigned char *)OPENSSL_malloc(len);
		if (data == NULL)
			{
			perror("OPENSSL_malloc");
			goto end;
			}
		printf("#ifndef HEADER_DH_H\n"
		       "#include <openssl/dh.h>\n"
		       "#endif\n");
		printf("DH *get_dh%d()\n\t{\n",bits);

		l=BN_bn2bin(dh->p,data);
		printf("\tstatic unsigned char dh%d_p[]={",bits);
		for (i=0; i<l; i++)
			{
			if ((i%12) == 0) printf("\n\t\t");
			printf("0x%02X,",data[i]);
			}
		printf("\n\t\t};\n");

		l=BN_bn2bin(dh->g,data);
		printf("\tstatic unsigned char dh%d_g[]={",bits);
		for (i=0; i<l; i++)
			{
			if ((i%12) == 0) printf("\n\t\t");
			printf("0x%02X,",data[i]);
			}
		printf("\n\t\t};\n");

		printf("\tDH *dh;\n\n");
		printf("\tif ((dh=DH_new()) == NULL) return(NULL);\n");
		printf("\tdh->p=BN_bin2bn(dh%d_p,sizeof(dh%d_p),NULL);\n",
			bits,bits);
		printf("\tdh->g=BN_bin2bn(dh%d_g,sizeof(dh%d_g),NULL);\n",
			bits,bits);
		printf("\tif ((dh->p == NULL) || (dh->g == NULL))\n");
		printf("\t\t{ DH_free(dh); return(NULL); }\n");
		if (dh->length)
			printf("\tdh->length = %ld;\n", dh->length);
		printf("\treturn(dh);\n\t}\n");
		OPENSSL_free(data);
		}


	if (!noout)
		{
		if 	(outformat == FORMAT_ASN1)
			i=i2d_DHparams_bio(out,dh);
		else if (dh->q)
			i=PEM_write_bio_DHxparams(out,dh);
		else
			i=PEM_write_bio_DHparams(out,dh);
		if (!i)
			{
			BIO_printf(bio_err,"unable to write DH parameters\n");
			ERR_print_errors(bio_err);
			goto end;
			}
		}
	ret=0;
end:
	if (in != NULL) BIO_free(in);
	if (out != NULL) BIO_free_all(out);
	if (dh != NULL) DH_free(dh);
	return(ret);
	}

/* dh_cb is identical to dsa_cb in apps/dsaparam.c */
static int dh_cb(int p, int n, BN_GENCB *cb)
	{
	char c='*';

	if (p == 0) c='.';
	if (p == 1) c='+';
	if (p == 2) c='*';
	if (p == 3) c='\n';
	BIO_write(cb->arg,&c,1);
	(void)BIO_flush(cb->arg);
#ifdef LINT
	p=n;
#endif
	return 1;
	}

#else /* !OPENSSL_NO_DH */

# if PEDANTIC
static void *dummy=&dummy;
# endif

#endif
