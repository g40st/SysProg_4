/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * rfc.h: Definitionen für das Netzwerkprotokoll gemäß dem RFC
 */

#ifndef RFC_H
#define RFC_H

#include "common/question.h"

#define RFC_VERSION_NUMBER 6
#define MAX_STRING_LENGTH 256

#define RFC_QUESTION_SHMEM_SIZE (QUESTION_SIZE + (NUM_ANSWERS * ANSWER_SIZE) + 2)

#pragma pack(push,1)

#define RFC_BASE_SIZE 5
/*
 * Every packet starts with this preamble.
 * It cointains the message ID and its length.
 */
struct rfcMain {
    char type[3];
    uint16_t length;
};

#define RFC_LRQ_SIZE (RFC_BASE_SIZE + 1)
struct rfcLoginRequest {
    struct rfcMain main; // Length: 1 + length of name
    uint8_t version; // This semester: 6
    char name[31]; // Name, not '\0'-terminated!
};

#define RFC_LOK_SIZE (RFC_BASE_SIZE + 2)
struct rfcLoginResponseOK {
    struct rfcMain main; // Length: 2
    uint8_t version; // This semester: 6
    uint8_t clientID; // Client ID (0 = Game master)
};

struct rfcCatalog {
    struct rfcMain main; // Length: 0 (end-marker) or length of filename
    char filename[MAX_STRING_LENGTH]; // Filename. Not '\0'-terminated!
};

struct rfcPlayer {
    char name[32]; // Name, not '\0'-terminated!
    uint32_t points; // Current score
    uint8_t id; // Player ID
};

#define RFC_LST_SIZE(x) (RFC_BASE_SIZE + ((x) * 37))
struct rfcPlayerList {
    struct rfcMain main; // Length: 37 * playerCount
    struct rfcPlayer players[4]; // Use only if length is big enough!
};

struct rfcStartGame {
    struct rfcMain main; // Length: length of filename
    char filename[MAX_STRING_LENGTH]; // Filename. Not '\0'-terminated!
};

#define RFC_QAN_SIZE (RFC_BASE_SIZE + 1)
struct rfcQuestionAnswered {
    struct rfcMain main; // Length: 1
    uint8_t selection; // Bitmask of selected answers
};

#define RFC_QUESTION_RESULT_SIZE (RFC_BASE_SIZE + 2)
struct rfcQuestionResult {
    struct rfcMain main; // Length: 2
    uint8_t timedOut; // Not zero when timeout was reached
    uint8_t correct; // Bitmask of correct answers
};

struct rfcGameOver {
    struct rfcMain main; // Length: 1
    uint8_t rank; // (1 <= rank <= 4)
};

#define RFC_ERR_SIZE (RFC_BASE_SIZE + 1)
struct rfcErrorWarning {
    struct rfcMain main; // Length: 1 + length of message
    uint8_t subtype; // 0 Warning; 1 Error, exit Client
    char message[MAX_STRING_LENGTH]; // Message. Not '\0'-terminated!
};

#define RFC_QUESTION_SIZE (QUESTION_SIZE + (NUM_ANSWERS * ANSWER_SIZE) + 1)
struct rfcQuestion {
    struct rfcMain main;
    QuestionMessage question;
};

/*
 * Big union containing all the possible messages.
 * This way, this single datastructure can be used
 * to comfortably send and receive every type of message.
 */
typedef union {
    struct rfcMain main;

    // Messages from Client to Server
    struct rfcLoginRequest loginRequest; // LRQ
    // CatalogRequest CRQ
    // QuestionRequest QRQ
    struct rfcQuestionAnswered questionAnswered; // QAN

    // Messages from Server to Client
    struct rfcLoginResponseOK loginResponseOK; // LOK
    struct rfcCatalog catalogResponse; // CRE
    struct rfcPlayerList playerList; // LST
    struct rfcQuestion question; // QUE
    struct rfcQuestionResult questionResult; // QRE
    struct rfcGameOver gameOver; // GOV
    struct rfcErrorWarning errorWarning; // ERR

    // Messages going in both directions
    struct rfcCatalog catalogChange; // CCH
    struct rfcStartGame startGame; // STG
} rfc;

#pragma pack(pop)

/*
 * Helper method to compare the message types.
 * Returns 1 if equal, 0 if not.
 */
int equalLiteral(struct rfcMain m, const char *s);

/*
 * Helper function to send an error or warning message.
 * Returns 1 on success, 0 on error.
 */
int sendErrorMessage(int socket, const char *message);
int sendWarningMessage(int socket, const char *message);

/*
 * Helper method to receive a single packet.
 * Returns 1 on success, 0 on connection close, -1 on error.
 */
int receivePacket(int socket, rfc *r);

#endif

