/*
  Copyright (C) CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifndef _WIN32
	#include <unistd.h>
#endif

#include "../libs/pilight/core/log.h"
#include "../libs/pilight/core/mem.h"
#include "../libs/pilight/config/settings.h"
#include "../libs/pilight/lua_c/lua.h"
#include "../libs/libuv/uv.h"
#include "mqtt.h"
#include "network.h"

#include "../../libuv/uv.h"

static char *type_to_string(int type) {
	switch(type) {
		case MQTT_PUBLISH:
			return "publish";
		case MQTT_SUBACK:
			return "suback";
		case MQTT_CONNECT:
			return "connect";
		case MQTT_SUBSCRIBE:
			return "subscribe";
		case MQTT_CONNACK:
			return "connack";
		case MQTT_PUBACK:
			return "puback";
		case MQTT_PUBREC:
			return "pubrec";
		case MQTT_PUBREL:
			return "pubrel";
		case MQTT_PUBCOMP:
			return "pubcomp";
	  case MQTT_PINGREQ:
			return "pingreq";
	  case MQTT_PINGRESP:
			return "pingresp";
		case MQTT_DISCONNECT:
			return "disconnect";
		case MQTT_DISCONNECTED:
			return "disconnected";
	}
	return "unknown";
}

void mqtt_dump(struct mqtt_pkt_t *pkt) {
	printf("-------------------\n");
	printf("type: %s\n", type_to_string(pkt->type));

	if(pkt->type == MQTT_PUBLISH ||
		 pkt->type == MQTT_SUBACK) {
		printf("dub: %d\n", pkt->dub);
		printf("qos: %d\n", pkt->qos);
		printf("retain: %d\n", pkt->retain);
	}
	if(pkt->type == MQTT_CONNECT ||
	   pkt->type == MQTT_SUBSCRIBE ||
	   pkt->type == MQTT_CONNACK ||
		 pkt->type == MQTT_PUBACK ||
		 pkt->type == MQTT_PUBREC ||
		 pkt->type == MQTT_PUBREL ||
	   pkt->type == MQTT_PINGREQ ||
	   pkt->type == MQTT_PINGRESP ||
		 pkt->type == MQTT_DISCONNECT) {
		printf("reserved: %d\n", pkt->reserved);
	}
	printf("\n");
	switch(pkt->type) {
		case MQTT_CONNECT: {
			printf("header\n");
			printf("\tprotocol name: %s\n", pkt->header.connect.protocol);
			printf("\tversion: %d\n", pkt->header.connect.version);
			printf("\tusername: %d\n", pkt->header.connect.username);
			printf("\tpassword: %d\n", pkt->header.connect.password);
			printf("\twill retain: %d\n", pkt->header.connect.willretain);
			printf("\tqos: %d\n", pkt->header.connect.qos);
			printf("\twill flag: %d\n", pkt->header.connect.willflag);
			printf("\tclean session: %d\n", pkt->header.connect.cleansession);
			printf("\treserved: %d\n", pkt->header.connect.reserved);

			printf("\n");
			printf("payload\n");
			printf("\tkeepalive: %d\n", pkt->payload.connect.keepalive);
			printf("\tclientid: %s\n", pkt->payload.connect.clientid);
			if(pkt->payload.connect.willtopic != NULL) {
				printf("\twill topic: %s\n", pkt->payload.connect.willtopic);
			}
			if(pkt->payload.connect.willmessage != NULL) {
				printf("\twill message: %s\n", pkt->payload.connect.willmessage);
			}
			if(pkt->header.connect.username == 1) {
				printf("\tusername: %s\n", pkt->payload.connect.username);
			}
			if(pkt->header.connect.password == 1) {
				printf("\tpassword: %s\n", pkt->payload.connect.password);
			}
		} break;
		case MQTT_CONNACK: {
			printf("header\n");
			printf("\treserved: %d\n", pkt->header.connack.reserved);
			printf("\tsession: %d\n", pkt->header.connack.session);

			printf("payload\n");
			printf("\tcode: %d\n", pkt->payload.connack.code);
		} break;
		case MQTT_PUBLISH: {
			printf("payload\n");
			if(pkt->qos > 0) {
				printf("\tmsgid: %d\n", pkt->payload.publish.msgid);
			}
			printf("\ttopic: %s\n", pkt->payload.publish.topic);
			printf("\tmessage: %s\n", pkt->payload.publish.message);
		} break;
		case MQTT_PUBACK: {
			printf("payload\n");
			printf("\tmsgid: %d\n", pkt->payload.puback.msgid);
		} break;
		case MQTT_PUBREC: {
			printf("payload\n");
			printf("\tmsgid: %d\n", pkt->payload.pubrec.msgid);
		} break;
		case MQTT_PUBREL: {
			printf("payload\n");
			printf("\tmsgid: %d\n", pkt->payload.pubrel.msgid);
		} break;
		case MQTT_PUBCOMP: {
			printf("payload\n");
			printf("\tmsgid: %d\n", pkt->payload.pubcomp.msgid);
		} break;
		case MQTT_SUBSCRIBE: {
			printf("payload\n");
			printf("\tmsgid: %d\n", pkt->payload.subscribe.msgid);
			printf("\ttopic: %s\n", pkt->payload.subscribe.topic);
			printf("\tqos: %d\n", pkt->payload.subscribe.qos);
		} break;
		case MQTT_SUBACK: {
			printf("payload\n");
			printf("\tmsgid: %d\n", pkt->payload.suback.msgid);
			printf("\tqos: %d\n", pkt->payload.suback.qos);
		} break;
	}
	printf("-------------------\n");
}
/*LCOV_EXCL_STOP*/

void mqtt_free(struct mqtt_pkt_t *pkt) {
	switch(pkt->type) {
		case MQTT_CONNECT: {
			if(pkt->header.connect.protocol != NULL) {
				FREE(pkt->header.connect.protocol);
			}
			if(pkt->payload.connect.username != NULL) {
				FREE(pkt->payload.connect.username);
			}
			if(pkt->payload.connect.password != NULL) {
				FREE(pkt->payload.connect.password);
			}
			if(pkt->payload.connect.clientid != NULL) {
				FREE(pkt->payload.connect.clientid);
			}
			if(pkt->payload.connect.willtopic != NULL) {
				FREE(pkt->payload.connect.willtopic);
			}
			if(pkt->payload.connect.willmessage != NULL) {
				FREE(pkt->payload.connect.willmessage);
			}
		} break;
		case MQTT_PUBLISH: {
			if(pkt->payload.publish.topic != NULL) {
				FREE(pkt->payload.publish.topic);
			}
			if(pkt->payload.publish.message != NULL) {
				FREE(pkt->payload.publish.message);
			}
		} break;
		case MQTT_SUBSCRIBE: {
			int i = 0;
			if(pkt->payload.subscribe.topic != NULL) {
				FREE(pkt->payload.subscribe.topic);
			}
			for(i=0;i<pkt->payload.subscribe.nrtopics;i++) {
				if(pkt->payload.subscribe.topics[i] != NULL) {
					FREE(pkt->payload.subscribe.topics[i]);
				}
			}
			if(pkt->payload.subscribe.topics != NULL) {
				FREE(pkt->payload.subscribe.topics);
			}
			if(pkt->payload.subscribe.options != NULL) {
				FREE(pkt->payload.subscribe.options);
			}
		} break;
		/* suback/unsuback codes are owned by the caller (encode only) */
		case MQTT_UNSUBSCRIBE: {
			int i = 0;
			for(i=0;i<pkt->payload.unsubscribe.nrtopics;i++) {
				if(pkt->payload.unsubscribe.topics[i] != NULL) {
					FREE(pkt->payload.unsubscribe.topics[i]);
				}
			}
			if(pkt->payload.unsubscribe.topics != NULL) {
				FREE(pkt->payload.unsubscribe.topics);
			}
		} break;
	}
}

/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, int *result) {
	int slen, tlen;
	int spos, tpos;
	int multilevel_wildcard = 0;

	if(!sub || !topic || !result) {
		return -1;
	}

	slen = strlen(sub);
	tlen = strlen(topic);

	if(slen && tlen) {
		if((sub[0] == '$' && topic[0] != '$') || (topic[0] == '$' && sub[0] != '$')) {
			*result = 0;
			return 0;
		}
	}

	spos = 0;
	tpos = 0;

	while(spos < slen && tpos < tlen) {
		if(sub[spos] == topic[tpos]) {
			if(tpos == tlen-1) {
				/* Check for e.g. foo matching foo/# */
				if(spos == slen-3
						&& sub[spos+1] == '/'
						&& sub[spos+2] == '#') {
					*result = 1;
					multilevel_wildcard = 1;
					return 0;
				}
			}
			spos++;
			tpos++;
			if(spos == slen && tpos == tlen){
				*result = 1;
				return 0;
			}else if(tpos == tlen && spos == slen-1 && sub[spos] == '+'){
				spos++;
				*result = 1;
				return 0;
			}
		} else {
			if(sub[spos] == '+') {
				spos++;
				while(tpos < tlen && topic[tpos] != '/') {
					tpos++;
				}
				if(tpos == tlen && spos == slen){
					*result = 1;
					return 0;
				}
			} else if(sub[spos] == '#') {
				multilevel_wildcard = 1;
				if(spos+1 != slen) {
					*result = 0;
					return 0;
				}else{
					*result = 1;
					return 0;
				}
			}else{
				*result = 0;
				return 0;
			}
		}
	}
	if(multilevel_wildcard == 0 && (tpos < tlen || spos < slen)) {
		*result = 0;
	}

	return 0;
}

static void read_value(unsigned char *buf, unsigned int len, char **val) {
	if((*val = MALLOC(len+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memcpy(*val, buf, len);
	(*val)[len] = '\0';
}

static void write_value(unsigned char **buf, unsigned int *len, char *val, int write_length) {
	int l = strlen(val), i = 0;

	if(write_length == 1) {
		(*buf)[(*len)++] = (l & 0xFF00) >> 8;
		(*buf)[(*len)++] = (l & 0xFF);
	}
	for(i=0;i<l;i++) {
		(*buf)[(*len)++] = val[i];
	}
}

/*
 * MQTT 5 CONNACK properties: features pilight's broker does not offer
 *   0x29 Subscription Identifier Available = 0
 *   0x2A Shared Subscription Available     = 0
 * (Topic Alias Maximum defaults to 0, i.e. no topic aliases.)
 */
static const unsigned char connack_props[] = { 0x29, 0x00, 0x2A, 0x00 };

int mqtt_encode(struct mqtt_pkt_t *pkt, unsigned char **buf, unsigned int *len) {
	*len = 0;
	unsigned int msglen = 0;
	int v5 = (pkt->version == 5);

	switch(pkt->type) {
		case MQTT_CONNECT: {
			msglen += 4;
			msglen += strlen(pkt->header.connect.protocol)+2;
			msglen += strlen(pkt->payload.connect.clientid)+2;
			if(pkt->payload.connect.willtopic != NULL) {
				msglen += strlen(pkt->payload.connect.willtopic)+2;
			}
			if(pkt->payload.connect.willmessage != NULL) {
				msglen += strlen(pkt->payload.connect.willmessage)+2;
			}
			if(pkt->header.connect.username == 1) {
				msglen += strlen(pkt->payload.connect.username)+2;
			}
			if(pkt->header.connect.password == 1) {
				msglen += strlen(pkt->payload.connect.password)+2;
			}
		} break;
		case MQTT_CONNACK: {
			msglen += 2;
			if(v5) {
				msglen += 1 + sizeof(connack_props);
			}
		} break;
		case MQTT_PUBLISH: {
			msglen += strlen(pkt->payload.publish.topic)+2;
			if(pkt->qos > 0) {
				msglen += 2;
			}
			if(v5) {
				msglen += 1;
			}
			msglen += strlen(pkt->payload.publish.message);
		} break;
		case MQTT_PUBACK:
		case MQTT_PUBREC:
		case MQTT_PUBREL:
		case MQTT_PUBCOMP: {
			msglen += 2;
		} break;
		case MQTT_SUBSCRIBE: {
			msglen += 2;
			msglen += strlen(pkt->payload.subscribe.topic)+2;
			msglen += 1;
		} break;
		case MQTT_SUBACK: {
			msglen += 2 + (v5 ? 1 : 0);
			msglen += (pkt->payload.suback.nrcodes > 0) ? pkt->payload.suback.nrcodes : 1;
		} break;
		case MQTT_UNSUBACK: {
			msglen += 2;
			if(v5) {
				msglen += 1 + pkt->payload.unsuback.nrcodes;
			}
		} break;
	}

	/* fixed header: 1 byte + up to 4 bytes remaining length */
	if(((*buf) = REALLOC((*buf), msglen+5)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset((*buf), 0, msglen+5);

	(*buf)[*len] = (pkt->type & 0x0F) << 4;

	if(pkt->type == MQTT_PUBLISH) {
		(*buf)[*len] |= (pkt->dub & 0x01) << 3;
		(*buf)[*len] |= (pkt->qos & 0x03) << 1;
		(*buf)[*len] |= pkt->retain & 0x01;
	}
	/* fixed header flags required by the specification */
	if(pkt->type == MQTT_SUBSCRIBE ||
		 pkt->type == MQTT_UNSUBSCRIBE ||
		 pkt->type == MQTT_PUBREL) {
		(*buf)[*len] |= 0x02;
	}
	(*len)++;

	unsigned int foo = msglen, msgbytes = 0;
	do {
		msgbytes++;
		unsigned char d = msglen % 128;
		msglen /= 128;

		/* if there are more digits to encode, set the top bit of this digit */
		if(msglen > 0) {
			d |= 0x80;
		}

		(*buf)[(*len)++] = d;
	} while (msglen > 0);

	switch(pkt->type) {
		case MQTT_CONNECT: {
			write_value(buf, len, pkt->header.connect.protocol, 1);
			(*buf)[(*len)++] = pkt->header.connect.version & 0x0F;
			(*buf)[(*len)] = (pkt->header.connect.username & 0x01) << 7;
			(*buf)[(*len)] |= (pkt->header.connect.password & 0x01) << 6;
			(*buf)[(*len)] |= (pkt->header.connect.willretain & 0x01) << 5;
			(*buf)[(*len)] |= (pkt->header.connect.qos & 0x03) << 3;
			(*buf)[(*len)] |= (pkt->header.connect.willflag & 0x01) << 2;
			(*buf)[(*len)] |= (pkt->header.connect.cleansession & 0x01) << 1;
			(*buf)[(*len)++] |= (pkt->header.connect.reserved & 0x01);

			(*buf)[(*len)++] = (pkt->payload.connect.keepalive & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.connect.keepalive & 0xFF;

			write_value(buf, len, pkt->payload.connect.clientid, 1);
			if(pkt->payload.connect.willtopic != NULL) {
				write_value(buf, len, pkt->payload.connect.willtopic, 1);
			}
			if(pkt->payload.connect.willmessage != NULL) {
				write_value(buf, len, pkt->payload.connect.willmessage, 1);
			}

			if(pkt->header.connect.username == 1) {
				write_value(buf, len, pkt->payload.connect.username, 1);
			}
			if(pkt->header.connect.password == 1) {
				write_value(buf, len, pkt->payload.connect.password, 1);
			}
		} break;
		case MQTT_CONNACK: {
			(*buf)[(*len)++] = (pkt->header.connack.session & 0x01);
			(*buf)[(*len)++] = pkt->payload.connack.code & 0xFF;
			if(v5) {
				unsigned int i = 0;
				(*buf)[(*len)++] = sizeof(connack_props);
				for(i=0;i<sizeof(connack_props);i++) {
					(*buf)[(*len)++] = connack_props[i];
				}
			}
		} break;
		case MQTT_PUBLISH: {
			write_value(buf, len, pkt->payload.publish.topic, 1);
			if(pkt->qos > 0) {
				(*buf)[(*len)++] = (pkt->payload.publish.msgid & 0xFF00) >> 8;
				(*buf)[(*len)++] = pkt->payload.publish.msgid & 0xFF;
			}
			if(v5) {
				/* no properties */
				(*buf)[(*len)++] = 0;
			}
			write_value(buf, len, pkt->payload.publish.message, 0);
		} break;
		case MQTT_PUBACK: {
			(*buf)[(*len)++] = (pkt->payload.puback.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.puback.msgid & 0xFF;
		} break;
		case MQTT_PUBREC: {
			(*buf)[(*len)++] = (pkt->payload.pubrec.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.pubrec.msgid & 0xFF;
		} break;
		case MQTT_PUBREL: {
			(*buf)[(*len)++] = (pkt->payload.pubrel.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.pubrel.msgid & 0xFF;
		} break;
		case MQTT_PUBCOMP: {
			(*buf)[(*len)++] = (pkt->payload.pubcomp.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.pubcomp.msgid & 0xFF;
		} break;
		case MQTT_SUBSCRIBE: {
			(*buf)[(*len)++] = (pkt->payload.subscribe.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.subscribe.msgid & 0xFF;
			write_value(buf, len, pkt->payload.subscribe.topic, 1);
			(*buf)[(*len)++] = pkt->payload.subscribe.qos & 0x03;
		} break;
		case MQTT_SUBACK: {
			(*buf)[(*len)++] = (pkt->payload.suback.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.suback.msgid & 0xFF;
			if(v5) {
				(*buf)[(*len)++] = 0;
			}
			if(pkt->payload.suback.nrcodes > 0) {
				int i = 0;
				for(i=0;i<pkt->payload.suback.nrcodes;i++) {
					(*buf)[(*len)++] = pkt->payload.suback.codes[i];
				}
			} else {
				(*buf)[(*len)++] = pkt->payload.suback.qos & 0x03;
			}
		} break;
		case MQTT_UNSUBACK: {
			(*buf)[(*len)++] = (pkt->payload.unsuback.msgid & 0xFF00) >> 8;
			(*buf)[(*len)++] = pkt->payload.unsuback.msgid & 0xFF;
			if(v5) {
				int i = 0;
				(*buf)[(*len)++] = 0;
				for(i=0;i<pkt->payload.unsuback.nrcodes;i++) {
					(*buf)[(*len)++] = pkt->payload.unsuback.codes[i];
				}
			}
		} break;
	}

	assert(*len-(1+msgbytes) == foo);

	return 0;
}

/*
 * Read an MQTT variable byte integer (max. 4 bytes) at *pos, not beyond end.
 * Returns 0 on success, -1 if malformed.
 */
static int read_varint(unsigned char *buf, unsigned int *pos, unsigned int end, unsigned int *value) {
	unsigned int multiplier = 1, n = 0;
	*value = 0;
	do {
		if((*pos) >= end || n++ >= 4) {
			return -1;
		}
		*value += (buf[(*pos)] & 0x7F) * multiplier;
		multiplier *= 128;
	} while(buf[(*pos)++] >= 128);
	return 0;
}

/*
 * MQTT 5: skip a property block (length + properties). pilight does not
 * use any of the properties a client may send, so they are validated for
 * length only and ignored.
 */
static int skip_properties(unsigned char *buf, unsigned int *pos, unsigned int end) {
	unsigned int plen = 0;
	if(read_varint(buf, pos, end, &plen) != 0) {
		return -1;
	}
	if((*pos)+plen > end) {
		return -1;
	}
	(*pos) += plen;
	return 0;
}

/* Read a two byte length prefixed string, not beyond end */
static int read_string(unsigned char *buf, unsigned int *pos, unsigned int end, char **val) {
	unsigned int length = 0;
	if((*pos)+2 > end) {
		return -1;
	}
	length = (buf[(*pos)] << 8) | buf[(*pos)+1]; (*pos)+=2;
	if((*pos)+length > end) {
		return -1;
	}
	if(*val != NULL) {
		FREE(*val);
	}
	read_value(&buf[(*pos)], length, val);
	(*pos)+=length;
	return 0;
}

int mqtt_decode(struct mqtt_pkt_t **pkt, unsigned char *buf, unsigned int len, unsigned int *pos) {
	return mqtt_decode_version(pkt, buf, len, pos, 4);
}

/*
 * Decode one packet. "version" is the protocol level negotiated for this
 * connection (3, 4 or 5); a CONNECT packet carries its own level.
 *
 * Returns 0 on success, 1 when more data is needed, -1 when malformed.
 */
int mqtt_decode_version(struct mqtt_pkt_t **pkt, unsigned char *buf, unsigned int len, unsigned int *pos, int version) {
	unsigned int length = 0, startpos = 0, end = 0, hdrpos = (*pos);
	int v5 = (version == 5);

	if(len < (*pos)+2) {
		/*
		 * Packet too short for minimal MQTT
		 */
		return 1;
	}

	if(((*pkt) = MALLOC(sizeof(struct mqtt_pkt_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset((*pkt), 0, sizeof(struct mqtt_pkt_t));

	(*pkt)->type = (buf[(*pos)] & 0xF0) >> 4;
	(*pkt)->version = version;
	if((*pkt)->type == MQTT_PUBLISH) {
		(*pkt)->dub = (buf[(*pos)] & 0x08) >> 3;
		(*pkt)->qos = (buf[(*pos)] & 0x06) >> 1;
		(*pkt)->retain = (buf[(*pos)] & 0x01);
	} else {
		(*pkt)->reserved = buf[(*pos)] & 0x0F;
	}
	(*pos)++;

	unsigned int msglength = 0;
	{
		unsigned int p = (*pos);
		if(read_varint(buf, &p, len, &msglength) != 0) {
			int incomplete = (p >= len && p-(*pos) < 4);
			mqtt_free((*pkt));
			FREE((*pkt));
			(*pos) = hdrpos;
			return incomplete ? 1 : -1;
		}
		(*pos) = p;
	}

	startpos = (*pos);
	end = startpos + msglength;

	if(len < end) {
		/*
		 * Packet too short for full message
		 * so we are waiting for additional
		 * content. Not an error and also not
		 * a successful decode, so a return 1.
		 */
		mqtt_free((*pkt));
		FREE((*pkt));
		(*pos) = hdrpos;
		return 1;
	}

#define NEED(n) if((*pos)+(n) > end) { goto malformed; }

	switch((*pkt)->type) {
		case MQTT_CONNECT: {
			if(read_string(buf, pos, end, &(*pkt)->header.connect.protocol) != 0) {
				goto malformed;
			}

			NEED(4);
			(*pkt)->header.connect.version = buf[(*pos)++];
			(*pkt)->header.connect.username = (buf[(*pos)] >> 7) & 0x01;
			(*pkt)->header.connect.password = (buf[(*pos)] >> 6) & 0x01;
			(*pkt)->header.connect.willretain = (buf[(*pos)] >> 5) & 0x01;
			(*pkt)->header.connect.qos = (buf[(*pos)] >> 3) & 0x03;
			(*pkt)->header.connect.willflag = (buf[(*pos)] >> 2) & 0x01;
			(*pkt)->header.connect.cleansession = (buf[(*pos)] >> 1) & 0x01;
			(*pkt)->header.connect.reserved = (buf[(*pos)++]) & 0x01;

			(*pkt)->payload.connect.keepalive = buf[(*pos)] << 8 | buf[(*pos)+1]; (*pos)+=2;

			/* the level announced in CONNECT applies to the packet itself */
			(*pkt)->version = (*pkt)->header.connect.version;
			v5 = ((*pkt)->version == 5);

			if(v5 && skip_properties(buf, pos, end) != 0) {
				goto malformed;
			}

			if(read_string(buf, pos, end, &(*pkt)->payload.connect.clientid) != 0) {
				goto malformed;
			}

			if((*pkt)->header.connect.willflag == 1) {
				if(v5 && skip_properties(buf, pos, end) != 0) {
					goto malformed;
				}
				if(read_string(buf, pos, end, &(*pkt)->payload.connect.willtopic) != 0) {
					goto malformed;
				}
				if(read_string(buf, pos, end, &(*pkt)->payload.connect.willmessage) != 0) {
					goto malformed;
				}
			}

			if((*pkt)->header.connect.username == 1) {
				if(read_string(buf, pos, end, &(*pkt)->payload.connect.username) != 0) {
					goto malformed;
				}
			}

			if((*pkt)->header.connect.password == 1) {
				if(read_string(buf, pos, end, &(*pkt)->payload.connect.password) != 0) {
					goto malformed;
				}
			}
		} break;
		case MQTT_CONNACK: {
			NEED(2);
			(*pkt)->header.connack.reserved = buf[(*pos)] >> 1;
			(*pkt)->header.connack.session = buf[(*pos)++] & 0x01;
			(*pkt)->payload.connack.code = buf[(*pos)++];
			if(v5 && (*pos) < end && skip_properties(buf, pos, end) != 0) {
				goto malformed;
			}
		} break;
		case MQTT_PUBLISH: {
			if(read_string(buf, pos, end, &(*pkt)->payload.publish.topic) != 0) {
				goto malformed;
			}

			if((*pkt)->qos > 0) {
				NEED(2);
				(*pkt)->payload.publish.msgid = (buf[(*pos)] << 8) | buf[(*pos)+1]; (*pos)+=2;
			}

			if(v5 && skip_properties(buf, pos, end) != 0) {
				goto malformed;
			}

			length = end-(*pos);
			read_value(&buf[(*pos)], length, &(*pkt)->payload.publish.message);
			(*pos)+=length;
		} break;
		case MQTT_PUBACK:
		case MQTT_PUBREC:
		case MQTT_PUBREL:
		case MQTT_PUBCOMP: {
			NEED(2);
			/* all four share the same layout */
			(*pkt)->payload.puback.msgid = (buf[(*pos)] << 8) | buf[(*pos)+1]; (*pos)+=2;
			/* MQTT 5: optional reason code and properties */
			if(v5) {
				(*pos) = end;
			}
		} break;
		case MQTT_SUBSCRIBE:
		case MQTT_UNSUBSCRIBE: {
			int sub = ((*pkt)->type == MQTT_SUBSCRIBE);
			NEED(2);
			uint16_t msgid = (buf[(*pos)] << 8) | buf[(*pos)+1]; (*pos)+=2;
			if(v5 && skip_properties(buf, pos, end) != 0) {
				goto malformed;
			}
			char ***topics = sub ? &(*pkt)->payload.subscribe.topics : &(*pkt)->payload.unsubscribe.topics;
			int *nr = sub ? &(*pkt)->payload.subscribe.nrtopics : &(*pkt)->payload.unsubscribe.nrtopics;
			while((*pos) < end) {
				if(((*topics) = REALLOC((*topics), sizeof(char *)*((*nr)+1))) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				(*topics)[(*nr)] = NULL;
				if(sub) {
					if(((*pkt)->payload.subscribe.options = REALLOC((*pkt)->payload.subscribe.options, (*nr)+1)) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
				}
				(*nr)++;
				if(read_string(buf, pos, end, &(*topics)[(*nr)-1]) != 0) {
					goto malformed;
				}
				if(sub) {
					NEED(1);
					(*pkt)->payload.subscribe.options[(*nr)-1] = buf[(*pos)++];
				}
			}
			if((*nr) == 0) {
				/* at least one topic filter is required */
				goto malformed;
			}
			if(sub) {
				(*pkt)->payload.subscribe.msgid = msgid;
				/* keep the single-topic fields for existing users */
				if(((*pkt)->payload.subscribe.topic = STRDUP((*topics)[0])) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				(*pkt)->payload.subscribe.qos = (*pkt)->payload.subscribe.options[0] & 0x03;
			} else {
				(*pkt)->payload.unsubscribe.msgid = msgid;
			}
		} break;
		case MQTT_SUBACK: {
			NEED(2);
			(*pkt)->payload.suback.msgid = (buf[(*pos)] << 8) | buf[(*pos)+1]; (*pos)+=2;
			if(v5 && skip_properties(buf, pos, end) != 0) {
				goto malformed;
			}
			NEED(1);
			(*pkt)->payload.suback.qos = buf[(*pos)];
			(*pos) = end;
		} break;
		case MQTT_UNSUBACK: {
			NEED(2);
			(*pkt)->payload.unsuback.msgid = (buf[(*pos)] << 8) | buf[(*pos)+1];
			(*pos) = end;
		} break;
		case MQTT_PINGREQ:
		case MQTT_PINGRESP: {
		} break;
		case MQTT_DISCONNECT: {
			/* MQTT 5: optional reason code and properties */
			(*pos) = end;
		} break;
		default: {
			/* Unknown packet type (e.g. MQTT 5 AUTH): skip its body */
			(*pos) = end;
		} break;
	}

#undef NEED

	if(end != (*pos)) {
		goto malformed;
	}

	return 0;

malformed:
	logprintf(LOG_DEBUG, "mqtt: malformed packet of type %d", (*pkt)->type);
	mqtt_free((*pkt));
	FREE((*pkt));
	return -1;
}

int mosquitto_sub_topic_check(const char *str) {
	char c = '\0';
	int len = 0;

	while(str && str[0]) {
		if(str[0] == '+') {
			if((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/')) {
				return -1;
			}
		} else if(str[0] == '#') {
			if((c != '\0' && c != '/') || str[1] != '\0') {
				return -1;
			}
		}
		len++;
		c = str[0];
		str = &str[1];
	}

	if(len > 65535) {
		return -1;
	}

	return 0;
}

static int mqtt_pkt_pubxxx(struct mqtt_pkt_t *pkt, int type, int msgid) {
	pkt->type = type;

	switch(type) {
		case MQTT_PUBACK: {
			pkt->payload.puback.msgid = msgid;
		} break;
		case MQTT_PUBREC: {
			pkt->payload.pubrec.msgid = msgid;
		} break;
		case MQTT_PUBREL: {
			pkt->payload.pubrel.msgid = msgid;
		} break;
		case MQTT_PUBCOMP: {
			pkt->payload.pubcomp.msgid = msgid;
		} break;
	}

	return 0;
}

int mqtt_pkt_puback(struct mqtt_pkt_t *pkt, int msgid) {
	return mqtt_pkt_pubxxx(pkt, MQTT_PUBACK, msgid);
}

int mqtt_pkt_pubrec(struct mqtt_pkt_t *pkt, int msgid) {
	return mqtt_pkt_pubxxx(pkt, MQTT_PUBREC, msgid);
}

int mqtt_pkt_pubrel(struct mqtt_pkt_t *pkt, int msgid) {
	return mqtt_pkt_pubxxx(pkt, MQTT_PUBREL, msgid);
}

int mqtt_pkt_pubcomp(struct mqtt_pkt_t *pkt, int msgid) {
	return mqtt_pkt_pubxxx(pkt, MQTT_PUBCOMP, msgid);
}

int mqtt_pkt_suback(struct mqtt_pkt_t *pkt, int msgid, int qos) {
	if(qos < 0 || qos > 2) {
		return -1;
	}

	pkt->type = MQTT_SUBACK;

	pkt->payload.suback.msgid = msgid;
	pkt->payload.suback.qos = qos;

	return 0;
}

int mqtt_pkt_connack(struct mqtt_pkt_t *pkt, enum mqtt_connection_t status) {
	pkt->type = MQTT_CONNACK;
	pkt->reserved = 0;

	pkt->payload.connack.code = (int)status;

	return 0;
}

int mqtt_pkt_connect(struct mqtt_pkt_t *pkt, int dub, int qos, int retain, char *clientid, char *protocol, int version, char *username, char *password, int willretain, int willflag, int cleansession, int keepalive, char *topic, char *message) {
	if(qos < 0 || qos > 2) {
		return -1;
	}
	if(protocol == NULL || clientid == NULL) {
		return -1;
	}

	pkt->type = MQTT_CONNECT;
	pkt->dub = dub;
	pkt->qos = qos;
	pkt->retain = retain;

	if((pkt->header.connect.protocol = STRDUP(protocol)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	pkt->header.connect.version = version;
	pkt->header.connect.username = (username != NULL);
	pkt->header.connect.password = (password != NULL);
	pkt->header.connect.willretain = willretain;
	pkt->header.connect.qos = qos;
	pkt->header.connect.willflag = willflag;
	pkt->header.connect.cleansession = cleansession;
	pkt->header.connect.reserved = 0;

	pkt->payload.connect.keepalive = keepalive;

	if((pkt->payload.connect.clientid = STRDUP(clientid)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	if(topic != NULL) {
		if((pkt->payload.connect.willtopic = STRDUP(topic)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
	}

	if(message != NULL) {
		if((pkt->payload.connect.willmessage = STRDUP(message)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
	}

	if(username != NULL) {
		if((pkt->payload.connect.username = STRDUP(username)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
	}

	if(password != NULL) {
		if((pkt->payload.connect.password = STRDUP(password)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
	}
	return 0;
}

int mqtt_ping(struct mqtt_client_t *client) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_pingreq(&pkt);
		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_subscribe(struct mqtt_client_t *client, char *topic, int qos) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));
		client->msgid = (client->msgid + 1) % 1024;

		mqtt_pkt_subscribe(&pkt, topic, client->msgid, qos);
		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_pubrec(struct mqtt_client_t *client, int msgid) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_pubrec(&pkt, msgid);

		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_pubrel(struct mqtt_client_t *client, int msgid) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_pubrel(&pkt, msgid);

		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_pubcomp(struct mqtt_client_t *client, int msgid) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_pubcomp(&pkt, msgid);

		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_puback(struct mqtt_client_t *client, int msgid) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_puback(&pkt, msgid);

		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_suback(struct mqtt_client_t *client, int msgid, int qos) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_suback(&pkt, msgid, qos);

		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_suback_codes(struct mqtt_client_t *client, int msgid, uint8_t *codes, int nrcodes) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		pkt.type = MQTT_SUBACK;
		pkt.version = client->version;
		pkt.payload.suback.msgid = msgid;
		pkt.payload.suback.codes = codes;
		pkt.payload.suback.nrcodes = nrcodes;

		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_unsuback(struct mqtt_client_t *client, int msgid, uint8_t *codes, int nrcodes) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		pkt.type = MQTT_UNSUBACK;
		pkt.version = client->version;
		pkt.payload.unsuback.msgid = msgid;
		pkt.payload.unsuback.codes = codes;
		pkt.payload.unsuback.nrcodes = nrcodes;

		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_disconnect(struct mqtt_client_t *client) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_disconnect(&pkt);
		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_pingresp(struct mqtt_client_t *client) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));

		mqtt_pkt_pingresp(&pkt);
		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		uv_async_send(client->async_req);
	}
	return 0;
}

static void mqtt_message_resend(uv_timer_t *handle) {
	struct mqtt_message_t *message = handle->data;
	uv_poll_t *poll_req = message->client->poll_req;
	struct uv_custom_poll_t *custom_poll_data = poll_req->data;

	struct mqtt_pkt_t pkt1;
	unsigned int len = 0;
	unsigned char *buf = NULL;
	memset(&pkt1, 0, sizeof(struct mqtt_pkt_t));

	mqtt_pkt_publish(&pkt1, 1, message->qos, 0, message->topic, message->msgid, message->payload);
	pkt1.version = message->client->version;

	if(mqtt_encode(&pkt1, &buf, &len) == 0) {
		iobuf_append(&custom_poll_data->send_iobuf, (void *)buf, len);
		uv_custom_write(poll_req);
		FREE(buf);
	}
	mqtt_free(&pkt1);
}

int mqtt_publish(struct mqtt_client_t *client, int dub, int qos, int retain, char *topic, char *payload) {
	if(client == NULL || client->step != MQTT_CONNECTED) {
		return -1;
	} else {
		struct mqtt_pkt_t pkt;
		unsigned char *buf = NULL;
		unsigned int len = 0;
		memset(&pkt, 0, sizeof(struct mqtt_pkt_t));
		client->msgid = (client->msgid + 1) % 1024;

		mqtt_pkt_publish(&pkt, dub, qos, retain, topic, client->msgid, payload);

		if(qos > 0) {
			struct mqtt_message_t *message = &client->messages[client->msgid];

			if((message->payload = STRDUP(payload)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			if((message->topic = STRDUP(topic)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			message->msgid = client->msgid;
			message->qos = qos;
			if(qos == 1) {
				message->step = MQTT_PUBACK;
			}
			if(qos == 2) {
				message->step = MQTT_PUBREC;
			}
			if((message->timer_req = MALLOC(sizeof(uv_timer_t))) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}

			message->timer_req->data = message;
			message->client = client;

			uv_timer_init(uv_default_loop(), message->timer_req);
#ifdef PILIGHT_UNITTEST
			uv_timer_start(message->timer_req, (void (*)(uv_timer_t *))mqtt_message_resend, 150, 150);
#else
			uv_timer_start(message->timer_req, (void (*)(uv_timer_t *))mqtt_message_resend, 3000, 3000);
#endif
		}

		pkt.version = client->version;
		if(mqtt_encode(&pkt, &buf, &len) == 0) {
			iobuf_append(&client->send_iobuf, (void *)buf, len);
			FREE(buf);
		}
		mqtt_free(&pkt);
		uv_async_send(client->async_req);
	}
	return 0;
}

int mqtt_pkt_subscribe(struct mqtt_pkt_t *pkt, char *topic, int msgid, int qos) {
	if(qos < 0 || qos > 2) {
		return -1;
	}
	if(topic == NULL) {
		return -1;
	}

	pkt->type = MQTT_SUBSCRIBE;

	if((pkt->payload.subscribe.topic = STRDUP(topic)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	pkt->payload.subscribe.msgid = msgid;
	pkt->payload.subscribe.qos = qos;

	return 0;
}

int mqtt_pkt_publish(struct mqtt_pkt_t *pkt, int dub, int qos, int retain, char *topic, int msgid, char *message) {
	if(qos < 0 || qos > 2) {
		return -1;
	}
	if(topic == NULL || message == NULL) {
		return -1;
	}

	pkt->type = MQTT_PUBLISH;
	pkt->dub = dub;
	pkt->qos = qos;
	pkt->retain = retain;

	if((pkt->payload.publish.topic = STRDUP(topic)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	pkt->payload.publish.msgid = msgid;

	if((pkt->payload.publish.message = STRDUP(message)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	return 0;
}

int mqtt_pkt_pingreq(struct mqtt_pkt_t *pkt) {
	pkt->type = MQTT_PINGREQ;

	return 0;
}

int mqtt_pkt_pingresp(struct mqtt_pkt_t *pkt) {
	pkt->type = MQTT_PINGRESP;

	return 0;
}

int mqtt_pkt_disconnect(struct mqtt_pkt_t *pkt) {
	pkt->type = MQTT_DISCONNECT;

	return 0;
}
