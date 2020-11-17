#ifndef SRC_INSTRUCTIONS_H_
#define SRC_INSTRUCTIONS_H_

// OP codes

// 8 bit load instructions.
#define LD_A_d8 0x3E
#define LD_B_d8 0x06
#define LD_C_d8 0x0E
#define LD_D_d8 0x16
#define LD_E_d8 0x1E
#define LD_H_d8 0x26
#define LD_L_d8 0x2E


#define LD_A_A 0x7F
#define LD_A_B 0x78
#define LD_A_C 0x79
#define LD_A_D 0x7A
#define LD_A_E 0x7B
#define LD_A_H 0x7C
#define LD_A_L 0x7D

#define LD_B_A 0x47
#define LD_B_B 0x40
#define LD_B_C 0x41
#define LD_B_D 0x42
#define LD_B_E 0x43
#define LD_B_H 0x44
#define LD_B_L 0x45

#define LD_C_A 0x4F
#define LD_C_B 0x48
#define LD_C_C 0x49
#define LD_C_D 0x4A
#define LD_C_E 0x4B
#define LD_C_H 0x4C
#define LD_C_L 0x4D

#define LD_D_A 0x57
#define LD_D_B 0x50
#define LD_D_C 0x51
#define LD_D_D 0x52
#define LD_D_E 0x53
#define LD_D_H 0x54
#define LD_D_L 0x55

#define LD_E_A 0x5F
#define LD_E_B 0x58
#define LD_E_C 0x59
#define LD_E_D 0x5A
#define LD_E_E 0x5B
#define LD_E_H 0x5C
#define LD_E_L 0x5D

#define LD_H_A 0x67
#define LD_H_B 0x60
#define LD_H_C 0x61
#define LD_H_D 0x62
#define LD_H_E 0x63
#define LD_H_H 0x64
#define LD_H_L 0x65

#define LD_L_A 0x6F
#define LD_L_B 0x68
#define LD_L_C 0x69
#define LD_L_D 0x6A
#define LD_L_E 0x6B
#define LD_L_H 0x6C
#define LD_L_L 0x6D

// Load from address (rr) instructions.
#define LD_A_BC 0x0A
#define LD_A_DE 0x1A
#define LD_A_HL 0x7E
#define LD_B_HL 0x46
#define LD_C_HL 0x4E
#define LD_D_HL 0x56
#define LD_E_HL 0x5E
#define LD_H_HL 0x66
#define LD_L_HL 0x6E

// Load value in r into address (rr).
#define LD_BC_A 0x02
#define LD_DE_A 0x12
#define LD_HL_A 0x77
#define LD_HL_B 0x70
#define LD_HL_C 0x71
#define LD_HL_D 0x72
#define LD_HL_E 0x73
#define LD_HL_H 0x74
#define LD_HL_L 0x75

// Direct load 8 bit value into address (HL).
#define LD_HL_d8 0x36

// Load 8 bit value from direct address (a16).
#define LD_A_a16 0xFA

// Load 8 bit value from r into direct address (a16).
#define LD_a16_A 0xEA

// Load 8 bit value into r1 from direct address (0xFF00 + r2).
#define LD_A_addrC 0xF2

// Load 8 bit value from r2 into direct address (0xFF00 + r1).
#define LD_addrC_A 0xE2

// Load 8 bit value from address (HL) into A, decrement HL.
#define LDD_A_HL 0x3A

// Load 8 bit value from A into address (HL), decrement HL.
#define LDD_HL_A 0x32

// Load 8 bit value from address (HL) into A, increment HL.
#define LDI_A_HL 0x2A

// Load 8 bit value from A into address (HL), increment HL.
#define LDI_HL_A 0x22

// Put A into memory address 0xFF00 + a8.
#define LDH_a8_A 0xE0

// Load value at 0xFF00 + a8 into A.
#define LDH_A_a8 0xF0



// 16 bit load instructions.

// 16 bit immediate loads.
#define LD_BC_d16 0x01
#define LD_DE_d16 0x11
#define LD_HL_d16 0x21
#define LD_SP_d16 0x31

#define LD_SP_HL 0xF9

// Put SP + d8 into HL.
#define LDHL_SP_d8 0xF8

// Put SP into address (a16).
#define LD_a16_SP 0x08

// PUSH instructions.
#define PUSH_AF 0xF5
#define PUSH_BC 0xC5
#define PUSH_DE 0xD5
#define PUSH_HL 0xE5

// POP instructions.
#define POP_AF 0xF1
#define POP_BC 0xC1
#define POP_DE 0xD1
#define POP_HL 0xE1



// 8 bit ALU instructions.

// 8 bit Add instructions.
#define ADD_A_A 0x87
#define ADD_A_B 0x80
#define ADD_A_C 0x81
#define ADD_A_D 0x82
#define ADD_A_E 0x83
#define ADD_A_H 0x84
#define ADD_A_L 0x85

// Add value at address (HL) to A.
#define ADD_A_HL 0x86

// Add immediate value d8 to A.
#define ADD_A_d8 0xC6

#define ADC_A_A 0x8F
#define ADC_A_B 0x88
#define ADC_A_C 0x89
#define ADC_A_D 0x8A
#define ADC_A_E 0x8B
#define ADC_A_H 0x8C
#define ADC_A_L 0x8D

// Add value at address (HL) + carry to A.
#define ADC_A_HL 0x8E

// Add immediate value d8 + carry to A.
#define ADC_A_d8 0xCE

// 8 bit Subtraction instructions.
#define SUB_A_A 0x97
#define SUB_A_B 0x90
#define SUB_A_C 0x91
#define SUB_A_D 0x92
#define SUB_A_E 0x93
#define SUB_A_H 0x94
#define SUB_A_L 0x95

// Subtract value at address (HL) from A.
#define SUB_A_HL 0x96

// Subtract immediate value d8 from A.
#define SUB_A_d8 0xD6

#define SBC_A_A 0x9F
#define SBC_A_B 0x98
#define SBC_A_C 0x99
#define SBC_A_D 0x9A
#define SBC_A_E 0x9B
#define SBC_A_H 0x9C
#define SBC_A_L 0x9D

// Subtract value at address (HL) + carry from A.
#define SBC_A_HL 0x9E

// Subtract immediate value d8 + carry from A.
#define SBC_A_d8 0xDE

// 8 bit And instructions.
#define AND_A_A 0xA7
#define AND_A_B 0xA0
#define AND_A_C 0xA1
#define AND_A_D 0xA2
#define AND_A_E 0xA3
#define AND_A_H 0xA4
#define AND_A_L 0xA5

// And value at address (HL) with A.
#define AND_A_HL 0xA6

// And immediate value d8 with A.
#define AND_A_d8 0xE6

// 8 bit Or instructions.
#define OR_A_A 0xB7
#define OR_A_B 0xB0
#define OR_A_C 0xB1
#define OR_A_D 0xB2
#define OR_A_E 0xB3
#define OR_A_H 0xB4
#define OR_A_L 0xB5

// Or value at address (HL) with A.
#define OR_A_HL 0xB6

// Or immediate value d8 with A.
#define OR_A_d8 0xF6

// 8 bit Xor instructions.
#define XOR_A_A 0xAF
#define XOR_A_B 0xA8
#define XOR_A_C 0xA9
#define XOR_A_D 0xAA
#define XOR_A_E 0xAB
#define XOR_A_H 0xAC
#define XOR_A_L 0xAD

// Xor value at address (HL) with A.
#define XOR_A_HL 0xAE

// Xor immediate value d8 with A.
#define XOR_A_d8 0xEE

// 8 bit Compare instructions.
#define CP_A_A 0xBF
#define CP_A_B 0xB8
#define CP_A_C 0xB9
#define CP_A_D 0xBA
#define CP_A_E 0xBB
#define CP_A_H 0xBC
#define CP_A_L 0xBD

// Compare value at address (HL) with A.
#define CP_A_HL 0xBE

// Compare immediate value d8 with A.
#define CP_A_d8 0xFE

// 8 bit Increment instructions.
#define INC_A 0x3C
#define INC_B 0x04
#define INC_C 0x0C
#define INC_D 0x14
#define INC_E 0x1C
#define INC_H 0x24
#define INC_L 0x2C

// Increment value at address (HL).
#define INC_aHL 0x34

// 8 bit Decrement instructions.
#define DEC_A 0x3D
#define DEC_B 0x05
#define DEC_C 0x0D
#define DEC_D 0x15
#define DEC_E 0x1D
#define DEC_H 0x25
#define DEC_L 0x2D

// Decrement value at address (HL).
#define DEC_aHL 0x35



// 16 bit ALU instructions.

// 16 bit add instructions.
#define ADD_HL_BC 0x09
#define ADD_HL_DE 0x19
#define ADD_HL_HL 0x29
#define ADD_HL_SP 0x39

// Add one byte signed immediate value d8 to SP.
#define ADD_SP_d8 0xE8

// 16 bit Increment instructions.
#define INC_BC 0x03
#define INC_DE 0x13
#define INC_HL 0x23
#define INC_SP 0x33

// 16 bit Decrement instructions.
#define DEC_BC 0x0B
#define DEC_DE 0x1B
#define DEC_HL 0x2B
#define DEC_SP 0x3B



// Miscellaneous instructions.

// Decimal adjust register A.
#define DAA 0x27

// Complement A register.
#define CPL 0x2F

// Complement carry flag.
#define CCF 0x3F

// Set carry flag.
#define SCF 0x37

// No operation.
#define NOP 0x00

#define HALT 0x76
#define STOP 0x10

// Disable interrupts.
#define DI 0xF3

// Enable interrupts.
#define EI 0xFB



// Rotate instructions.
#define RLCA 0x07
#define RLA 0x17
#define RRCA 0x0F
#define RRA 0x1F



// Jumps
#define JP_a16 0xC3

// Conditional Jumps.
#define JP_NZ_a16 0xC2
#define JP_Z_a16 0xCA
#define JP_NC_a16 0xD2
#define JP_C_a16 0xDA

// Jump to address stored in HL.
#define JP_HL 0xE9

// Add d8 to the current address and jump to it.
#define JR_d8 0x18

// Conditionally add d8 to the current address and jump to it.
#define JR_NZ_a16 0x20
#define JR_Z_a16 0x28
#define JR_NC_a16 0x30
#define JR_C_a16 0x38



// Calls
#define CALL_a16 0xCD

#define CALL_NZ_a16 0xC4
#define CALL_Z_a16 0xCC
#define CALL_NC_a16 0xD4
#define CALL_C_a16 0xDC



// Returns
#define RET 0xC9

#define RET_NZ 0xC0
#define RET_Z 0xC8
#define RET_NC 0xD0
#define RET_C 0xD8

#define RETI 0xD9



// Restarts
#define RST_00H 0xC7
#define RST_08H 0xCF
#define RST_10H 0xD7
#define RST_18H 0xDF
#define RST_20H 0xE7
#define RST_28H 0xEF
#define RST_30H 0xF7
#define RST_38H 0xFF



// CB prefix instructions.
#define CB_PREFIX 0xCB

// Swap instructions.
#define SWAP_A 0x37
#define SWAP_B 0x30
#define SWAP_C 0x31
#define SWAP_D 0x32
#define SWAP_E 0x33
#define SWAP_H 0x34
#define SWAP_L 0x35

// Swap upper and lower nibles of value at address (HL).
#define SWAP_HL 0x36

// Shift and Rotate instructions.
#define RLC_A 0x07
#define RLC_B 0x00
#define RLC_C 0x01
#define RLC_D 0x02
#define RLC_E 0x03
#define RLC_H 0x04
#define RLC_L 0x05

#define RLC_HL 0x06


#define RL_A 0x17
#define RL_B 0x10
#define RL_C 0x11
#define RL_D 0x12
#define RL_E 0x13
#define RL_H 0x14
#define RL_L 0x15

#define RL_HL 0x16


#define RRC_A 0x0F
#define RRC_B 0x08
#define RRC_C 0x09
#define RRC_D 0x0A
#define RRC_E 0x0B
#define RRC_H 0x0C
#define RRC_L 0x0D

#define RRC_HL 0x0E


#define RR_A 0x1F
#define RR_B 0x18
#define RR_C 0x19
#define RR_D 0x1A
#define RR_E 0x1B
#define RR_H 0x1C
#define RR_L 0x1D

#define RR_HL 0x1E


#define SLA_A 0x27
#define SLA_B 0x20
#define SLA_C 0x21
#define SLA_D 0x22
#define SLA_E 0x23
#define SLA_H 0x24
#define SLA_L 0x25

#define SLA_HL 0x26


#define SRA_A 0x2F
#define SRA_B 0x28
#define SRA_C 0x29
#define SRA_D 0x2A
#define SRA_E 0x2B
#define SRA_H 0x2C
#define SRA_L 0x2D

#define SRA_HL 0x2E


#define SRL_A 0x3F
#define SRL_B 0x38
#define SRL_C 0x39
#define SRL_D 0x3A
#define SRL_E 0x3B
#define SRL_H 0x3C
#define SRL_L 0x3D

#define SRL_HL 0x3E



// Bit opcodes
#define BIT_0_B 0x40
#define BIT_1_B 0x48
#define BIT_2_B 0x50
#define BIT_3_B 0x58
#define BIT_4_B 0x60
#define BIT_5_B 0x68
#define BIT_6_B 0x70
#define BIT_7_B 0x78

#define BIT_0_C 0x41
#define BIT_1_C 0x49
#define BIT_2_C 0x51
#define BIT_3_C 0x59
#define BIT_4_C 0x61
#define BIT_5_C 0x69
#define BIT_6_C 0x71
#define BIT_7_C 0x79

#define BIT_0_D 0x42
#define BIT_1_D 0x4A
#define BIT_2_D 0x52
#define BIT_3_D 0x5A
#define BIT_4_D 0x62
#define BIT_5_D 0x6A
#define BIT_6_D 0x72
#define BIT_7_D 0x7A

#define BIT_0_E 0x43
#define BIT_1_E 0x4B
#define BIT_2_E 0x53
#define BIT_3_E 0x5B
#define BIT_4_E 0x63
#define BIT_5_E 0x6B
#define BIT_6_E 0x73
#define BIT_7_E 0x7B

#define BIT_0_H 0x44
#define BIT_1_H 0x4C
#define BIT_2_H 0x54
#define BIT_3_H 0x5C
#define BIT_4_H 0x64
#define BIT_5_H 0x6C
#define BIT_6_H 0x74
#define BIT_7_H 0x7C

#define BIT_0_L 0x45
#define BIT_1_L 0x4D
#define BIT_2_L 0x55
#define BIT_3_L 0x5D
#define BIT_4_L 0x65
#define BIT_5_L 0x6D
#define BIT_6_L 0x75
#define BIT_7_L 0x7D

#define BIT_0_HL 0x46
#define BIT_1_HL 0x4E
#define BIT_2_HL 0x56
#define BIT_3_HL 0x5E
#define BIT_4_HL 0x66
#define BIT_5_HL 0x6E
#define BIT_6_HL 0x76
#define BIT_7_HL 0x7E

#define BIT_0_A 0x47
#define BIT_1_A 0x4F
#define BIT_2_A 0x57
#define BIT_3_A 0x5F
#define BIT_4_A 0x67
#define BIT_5_A 0x6F
#define BIT_6_A 0x77
#define BIT_7_A 0x7F


#define SET_0_B 0xC0
#define SET_1_B 0xC8
#define SET_2_B 0xD0
#define SET_3_B 0xD8
#define SET_4_B 0xE0
#define SET_5_B 0xE8
#define SET_6_B 0xF0
#define SET_7_B 0xF8

#define SET_0_C 0xC1
#define SET_1_C 0xC9
#define SET_2_C 0xD1
#define SET_3_C 0xD9
#define SET_4_C 0xE1
#define SET_5_C 0xE9
#define SET_6_C 0xF1
#define SET_7_C 0xF9

#define SET_0_D 0xC2
#define SET_1_D 0xCA
#define SET_2_D 0xD2
#define SET_3_D 0xDA
#define SET_4_D 0xE2
#define SET_5_D 0xEA
#define SET_6_D 0xF2
#define SET_7_D 0xFA

#define SET_0_E 0xC3
#define SET_1_E 0xCB
#define SET_2_E 0xD3
#define SET_3_E 0xDB
#define SET_4_E 0xE3
#define SET_5_E 0xEB
#define SET_6_E 0xF3
#define SET_7_E 0xFB

#define SET_0_H 0xC4
#define SET_1_H 0xCC
#define SET_2_H 0xD4
#define SET_3_H 0xDC
#define SET_4_H 0xE4
#define SET_5_H 0xEC
#define SET_6_H 0xF4
#define SET_7_H 0xFC

#define SET_0_L 0xC5
#define SET_1_L 0xCD
#define SET_2_L 0xD5
#define SET_3_L 0xDD
#define SET_4_L 0xE5
#define SET_5_L 0xED
#define SET_6_L 0xF5
#define SET_7_L 0xFD

#define SET_0_HL 0xC6
#define SET_1_HL 0xCE
#define SET_2_HL 0xD6
#define SET_3_HL 0xDE
#define SET_4_HL 0xE6
#define SET_5_HL 0xEE
#define SET_6_HL 0xF6
#define SET_7_HL 0xFE

#define SET_0_A 0xC7
#define SET_1_A 0xCF
#define SET_2_A 0xD7
#define SET_3_A 0xDF
#define SET_4_A 0xE7
#define SET_5_A 0xEF
#define SET_6_A 0xF7
#define SET_7_A 0xFF


#define RES_0_B 0x80
#define RES_1_B 0x88
#define RES_2_B 0x90
#define RES_3_B 0x98
#define RES_4_B 0xA0
#define RES_5_B 0xA8
#define RES_6_B 0xB0
#define RES_7_B 0xB8

#define RES_0_C 0x81
#define RES_1_C 0x89
#define RES_2_C 0x91
#define RES_3_C 0x99
#define RES_4_C 0xA1
#define RES_5_C 0xA9
#define RES_6_C 0xB1
#define RES_7_C 0xB9

#define RES_0_D 0x82
#define RES_1_D 0x8A
#define RES_2_D 0x92
#define RES_3_D 0x9A
#define RES_4_D 0xA2
#define RES_5_D 0xAA
#define RES_6_D 0xB2
#define RES_7_D 0xBA

#define RES_0_E 0x83
#define RES_1_E 0x8B
#define RES_2_E 0x93
#define RES_3_E 0x9B
#define RES_4_E 0xA3
#define RES_5_E 0xAB
#define RES_6_E 0xB3
#define RES_7_E 0xBB

#define RES_0_H 0x84
#define RES_1_H 0x8C
#define RES_2_H 0x94
#define RES_3_H 0x9C
#define RES_4_H 0xA4
#define RES_5_H 0xAC
#define RES_6_H 0xB4
#define RES_7_H 0xBC

#define RES_0_L 0x85
#define RES_1_L 0x8D
#define RES_2_L 0x95
#define RES_3_L 0x9D
#define RES_4_L 0xA5
#define RES_5_L 0xAD
#define RES_6_L 0xB5
#define RES_7_L 0xBD

#define RES_0_HL 0x86
#define RES_1_HL 0x8E
#define RES_2_HL 0x96
#define RES_3_HL 0x9E
#define RES_4_HL 0xA6
#define RES_5_HL 0xAE
#define RES_6_HL 0xB6
#define RES_7_HL 0xBE

#define RES_0_A 0x87
#define RES_1_A 0x8F
#define RES_2_A 0x97
#define RES_3_A 0x9F
#define RES_4_A 0xA7
#define RES_5_A 0xAF
#define RES_6_A 0xB7
#define RES_7_A 0xBF


#endif  // SRC_INSTRUCTIONS_H_
