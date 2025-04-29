	.global isr_stub_table
	.global syscall_return

	.extern idt_exception_handler
	.extern idt_pic_timer
	.extern idt_pic_keyboard
	.extern idt_pic_mouse
	.extern idt_pic_eoi
	.extern syscall_handler
	.extern current_pcb
	.extern isr_save

.macro PUSHALL
	# regs
	pushq	%rax
	pushq 	%rbx
	pushq 	%rcx
	pushq 	%rdx
	pushq 	%rsi
	pushq 	%rdi
	pushq 	%rbp
	pushq 	%r8
	pushq 	%r9
	pushq 	%r10
	pushq 	%r11
	pushq 	%r12
	pushq 	%r13
	pushq 	%r14
	pushq 	%r15

	# segments
	movw 	%ds, %ax
	pushw	%ax
	movw 	%es, %ax
	pushw	%ax
	movw 	%fs, %ax
	pushw	%ax
	movw 	%gs, %ax
	pushw	%ax

	# pgdir
	movq	%cr3, %rax
	pushq	%rax
.endm

.macro POPALL
	# pgdir
	popq	%rax
	movq	%rax, %cr3

	# segments
	popw	%ax
	movw 	%ax, %gs
	popw	%ax
	movw 	%ax, %fs
	popw	%ax
	movw 	%ax, %es
	popw	%ax
	movw 	%ax, %ds

	# regs
	popq	%r15
	popq 	%r14
	popq 	%r13
	popq 	%r12
	popq 	%r11
	popq 	%r10
	popq 	%r9
	popq 	%r8
	popq 	%rbp
	popq 	%rdi
	popq 	%rsi
	popq 	%rdx
	popq 	%rcx
	popq 	%rbx
	popq 	%rax
.endm

.macro ISRSave
	PUSHALL
	cld

	movq	%rsp, %rdi
	callq	isr_save
.endm

.macro ISRRestore
	POPALL
	iretq
.endm

# call the exception handler with the interrupt number
# args: interrupt number
.macro ISRException num
	.align 8
isr_stub_\num:
	ISRSave
	movq	$\num, %rdi				# exception number
	movq	$0, %rsi                # placeholder error code
	callq	idt_exception_handler
	ISRRestore
.endm

# call the exception handler with the interrupt number
# these exceptions also put an error code on the stack
# args: interrupt number
.macro ISRExceptionCode num
	.align 8
isr_stub_\num:
	# retrieve the error code without corrupting registers
	movq	%rax, isr_tmp
	popq	%rax
	movq	%rax, isr_err_code
	movq	isr_tmp, %rax
	ISRSave
	movq	$\num, %rdi             # exception number
	movq	isr_err_code, %rsi      # error code
	callq	idt_exception_handler
	ISRRestore
.endm

.macro SYSCALL num
	.align 8
isr_stub_\num:
	ISRSave
	callq	syscall_handler
	jmp		syscall_return
.endm

.macro PICGeneric num
	.align 8
isr_stub_\num:
	ISRSave
	movq	$\num, %rdi
	callq	idt_pic_eoi
	ISRRestore
.endm

# we have to send eoi first since
# idt_pic_timer may not return
.macro PICTimer num
	.align 8
isr_stub_\num:
	ISRSave
	callq	idt_pic_eoi
	movq	$\num, %rdi
	callq	idt_pic_timer
	ISRRestore
.endm

.macro PICKeyboard num
	.align 8
isr_stub_\num:
	ISRSave
	callq	idt_pic_keyboard
	movq	$\num, %rdi
	callq	idt_pic_eoi
	ISRRestore
.endm

.macro PICMouse num
	.align 8
isr_stub_\num:
	ISRSave
	callq	idt_pic_mouse
	movq	$\num, %rdi
	callq	idt_pic_eoi
	ISRRestore
.endm

# do nothing
# args: interrupt number
.macro ISRIgnore num
	.align 8
isr_stub_\num:
	iretq
.endm

# isr temp storage
	.section .data
isr_tmp:
	.quad 0
isr_err_code:
	.quad 0

# isr stub table
	.section .rodata
	.align 16
	.code64
isr_stub_table:
	.quad isr_stub_0
	.quad isr_stub_1
	.quad isr_stub_2
	.quad isr_stub_3
	.quad isr_stub_4
	.quad isr_stub_5
	.quad isr_stub_6
	.quad isr_stub_7
	.quad isr_stub_8
	.quad isr_stub_9
	.quad isr_stub_10
	.quad isr_stub_11
	.quad isr_stub_12
	.quad isr_stub_13
	.quad isr_stub_14
	.quad isr_stub_15
	.quad isr_stub_16
	.quad isr_stub_17
	.quad isr_stub_18
	.quad isr_stub_19
	.quad isr_stub_20
	.quad isr_stub_21
	.quad isr_stub_22
	.quad isr_stub_23
	.quad isr_stub_24
	.quad isr_stub_25
	.quad isr_stub_26
	.quad isr_stub_27
	.quad isr_stub_28
	.quad isr_stub_29
	.quad isr_stub_30
	.quad isr_stub_31
	.quad isr_stub_32
	.quad isr_stub_33
	.quad isr_stub_34
	.quad isr_stub_35
	.quad isr_stub_36
	.quad isr_stub_37
	.quad isr_stub_38
	.quad isr_stub_39
	.quad isr_stub_40
	.quad isr_stub_41
	.quad isr_stub_42
	.quad isr_stub_43
	.quad isr_stub_44
	.quad isr_stub_45
	.quad isr_stub_46
	.quad isr_stub_47
	.quad isr_stub_48
	.quad isr_stub_49
	.quad isr_stub_50
	.quad isr_stub_51
	.quad isr_stub_52
	.quad isr_stub_53
	.quad isr_stub_54
	.quad isr_stub_55
	.quad isr_stub_56
	.quad isr_stub_57
	.quad isr_stub_58
	.quad isr_stub_59
	.quad isr_stub_60
	.quad isr_stub_61
	.quad isr_stub_62
	.quad isr_stub_63
	.quad isr_stub_64
	.quad isr_stub_65
	.quad isr_stub_66
	.quad isr_stub_67
	.quad isr_stub_68
	.quad isr_stub_69
	.quad isr_stub_70
	.quad isr_stub_71
	.quad isr_stub_72
	.quad isr_stub_73
	.quad isr_stub_74
	.quad isr_stub_75
	.quad isr_stub_76
	.quad isr_stub_77
	.quad isr_stub_78
	.quad isr_stub_79
	.quad isr_stub_80
	.quad isr_stub_81
	.quad isr_stub_82
	.quad isr_stub_83
	.quad isr_stub_84
	.quad isr_stub_85
	.quad isr_stub_86
	.quad isr_stub_87
	.quad isr_stub_88
	.quad isr_stub_89
	.quad isr_stub_90
	.quad isr_stub_91
	.quad isr_stub_92
	.quad isr_stub_93
	.quad isr_stub_94
	.quad isr_stub_95
	.quad isr_stub_96
	.quad isr_stub_97
	.quad isr_stub_98
	.quad isr_stub_99
	.quad isr_stub_100
	.quad isr_stub_101
	.quad isr_stub_102
	.quad isr_stub_103
	.quad isr_stub_104
	.quad isr_stub_105
	.quad isr_stub_106
	.quad isr_stub_107
	.quad isr_stub_108
	.quad isr_stub_109
	.quad isr_stub_110
	.quad isr_stub_111
	.quad isr_stub_112
	.quad isr_stub_113
	.quad isr_stub_114
	.quad isr_stub_115
	.quad isr_stub_116
	.quad isr_stub_117
	.quad isr_stub_118
	.quad isr_stub_119
	.quad isr_stub_120
	.quad isr_stub_121
	.quad isr_stub_122
	.quad isr_stub_123
	.quad isr_stub_124
	.quad isr_stub_125
	.quad isr_stub_126
	.quad isr_stub_127
	.quad isr_stub_128
	.quad isr_stub_129
	.quad isr_stub_130
	.quad isr_stub_131
	.quad isr_stub_132
	.quad isr_stub_133
	.quad isr_stub_134
	.quad isr_stub_135
	.quad isr_stub_136
	.quad isr_stub_137
	.quad isr_stub_138
	.quad isr_stub_139
	.quad isr_stub_140
	.quad isr_stub_141
	.quad isr_stub_142
	.quad isr_stub_143
	.quad isr_stub_144
	.quad isr_stub_145
	.quad isr_stub_146
	.quad isr_stub_147
	.quad isr_stub_148
	.quad isr_stub_149
	.quad isr_stub_150
	.quad isr_stub_151
	.quad isr_stub_152
	.quad isr_stub_153
	.quad isr_stub_154
	.quad isr_stub_155
	.quad isr_stub_156
	.quad isr_stub_157
	.quad isr_stub_158
	.quad isr_stub_159
	.quad isr_stub_160
	.quad isr_stub_161
	.quad isr_stub_162
	.quad isr_stub_163
	.quad isr_stub_164
	.quad isr_stub_165
	.quad isr_stub_166
	.quad isr_stub_167
	.quad isr_stub_168
	.quad isr_stub_169
	.quad isr_stub_170
	.quad isr_stub_171
	.quad isr_stub_172
	.quad isr_stub_173
	.quad isr_stub_174
	.quad isr_stub_175
	.quad isr_stub_176
	.quad isr_stub_177
	.quad isr_stub_178
	.quad isr_stub_179
	.quad isr_stub_180
	.quad isr_stub_181
	.quad isr_stub_182
	.quad isr_stub_183
	.quad isr_stub_184
	.quad isr_stub_185
	.quad isr_stub_186
	.quad isr_stub_187
	.quad isr_stub_188
	.quad isr_stub_189
	.quad isr_stub_190
	.quad isr_stub_191
	.quad isr_stub_192
	.quad isr_stub_193
	.quad isr_stub_194
	.quad isr_stub_195
	.quad isr_stub_196
	.quad isr_stub_197
	.quad isr_stub_198
	.quad isr_stub_199
	.quad isr_stub_200
	.quad isr_stub_201
	.quad isr_stub_202
	.quad isr_stub_203
	.quad isr_stub_204
	.quad isr_stub_205
	.quad isr_stub_206
	.quad isr_stub_207
	.quad isr_stub_208
	.quad isr_stub_209
	.quad isr_stub_210
	.quad isr_stub_211
	.quad isr_stub_212
	.quad isr_stub_213
	.quad isr_stub_214
	.quad isr_stub_215
	.quad isr_stub_216
	.quad isr_stub_217
	.quad isr_stub_218
	.quad isr_stub_219
	.quad isr_stub_220
	.quad isr_stub_221
	.quad isr_stub_222
	.quad isr_stub_223
	.quad isr_stub_224
	.quad isr_stub_225
	.quad isr_stub_226
	.quad isr_stub_227
	.quad isr_stub_228
	.quad isr_stub_229
	.quad isr_stub_230
	.quad isr_stub_231
	.quad isr_stub_232
	.quad isr_stub_233
	.quad isr_stub_234
	.quad isr_stub_235
	.quad isr_stub_236
	.quad isr_stub_237
	.quad isr_stub_238
	.quad isr_stub_239
	.quad isr_stub_240
	.quad isr_stub_241
	.quad isr_stub_242
	.quad isr_stub_243
	.quad isr_stub_244
	.quad isr_stub_245
	.quad isr_stub_246
	.quad isr_stub_247
	.quad isr_stub_248
	.quad isr_stub_249
	.quad isr_stub_250
	.quad isr_stub_251
	.quad isr_stub_252
	.quad isr_stub_253
	.quad isr_stub_254
	.quad isr_stub_255

	.section .text
	.code64

# isr restore
syscall_return:
	// get current pcb address
	movq	current_pcb, %rbx

	// load user stack
	leaq	8(%rbx), %rsp

	// return
	POPALL
	iretq

# isr stubs
ISRException 0
ISRException 1
ISRException 2
ISRException 3
ISRException 4
ISRException 5
ISRException 6
ISRException 7
ISRExceptionCode 8
ISRException 9
ISRExceptionCode 10
ISRExceptionCode 11
ISRExceptionCode 12
ISRExceptionCode 13
ISRExceptionCode 14
ISRException 15
ISRException 16
ISRExceptionCode 17
ISRException 18
ISRException 19
ISRException 20
ISRExceptionCode 21
ISRException 22
ISRException 23
ISRException 24
ISRException 25
ISRException 26
ISRException 27
ISRException 28
ISRExceptionCode 29
ISRExceptionCode 30
ISRException 31

PICTimer    32 # 0
PICKeyboard 33 # 1
PICGeneric  34 # 2
PICGeneric  35 # 3
PICGeneric  36 # 4
PICGeneric  37 # 5
PICGeneric  38 # 6
PICGeneric  39 # 7
PICGeneric  40 # 8
PICGeneric  41 # 9
PICGeneric  42 # 10
PICGeneric  43 # 11
PICMouse    44 # 12
PICGeneric  45 # 13
PICGeneric  46 # 14
PICGeneric  47 # 15

ISRIgnore 48
ISRIgnore 49
ISRIgnore 50
ISRIgnore 51
ISRIgnore 52
ISRIgnore 53
ISRIgnore 54
ISRIgnore 55
ISRIgnore 56
ISRIgnore 57
ISRIgnore 58
ISRIgnore 59
ISRIgnore 60
ISRIgnore 61
ISRIgnore 62
ISRIgnore 63
ISRIgnore 64
ISRIgnore 65
ISRIgnore 66
ISRIgnore 67
ISRIgnore 68
ISRIgnore 69
ISRIgnore 70
ISRIgnore 71
ISRIgnore 72
ISRIgnore 73
ISRIgnore 74
ISRIgnore 75
ISRIgnore 76
ISRIgnore 77
ISRIgnore 78
ISRIgnore 79
ISRIgnore 80
ISRIgnore 81
ISRIgnore 82
ISRIgnore 83
ISRIgnore 84
ISRIgnore 85
ISRIgnore 86
ISRIgnore 87
ISRIgnore 88
ISRIgnore 89
ISRIgnore 90
ISRIgnore 91
ISRIgnore 92
ISRIgnore 93
ISRIgnore 94
ISRIgnore 95
ISRIgnore 96
ISRIgnore 97
ISRIgnore 98
ISRIgnore 99
ISRIgnore 100
ISRIgnore 101
ISRIgnore 102
ISRIgnore 103
ISRIgnore 104
ISRIgnore 105
ISRIgnore 106
ISRIgnore 107
ISRIgnore 108
ISRIgnore 109
ISRIgnore 110
ISRIgnore 111
ISRIgnore 112
ISRIgnore 113
ISRIgnore 114
ISRIgnore 115
ISRIgnore 116
ISRIgnore 117
ISRIgnore 118
ISRIgnore 119
ISRIgnore 120
ISRIgnore 121
ISRIgnore 122
ISRIgnore 123
ISRIgnore 124
ISRIgnore 125
ISRIgnore 126
ISRIgnore 127
SYSCALL 128
ISRIgnore 129
ISRIgnore 130
ISRIgnore 131
ISRIgnore 132
ISRIgnore 133
ISRIgnore 134
ISRIgnore 135
ISRIgnore 136
ISRIgnore 137
ISRIgnore 138
ISRIgnore 139
ISRIgnore 140
ISRIgnore 141
ISRIgnore 142
ISRIgnore 143
ISRIgnore 144
ISRIgnore 145
ISRIgnore 146
ISRIgnore 147
ISRIgnore 148
ISRIgnore 149
ISRIgnore 150
ISRIgnore 151
ISRIgnore 152
ISRIgnore 153
ISRIgnore 154
ISRIgnore 155
ISRIgnore 156
ISRIgnore 157
ISRIgnore 158
ISRIgnore 159
ISRIgnore 160
ISRIgnore 161
ISRIgnore 162
ISRIgnore 163
ISRIgnore 164
ISRIgnore 165
ISRIgnore 166
ISRIgnore 167
ISRIgnore 168
ISRIgnore 169
ISRIgnore 170
ISRIgnore 171
ISRIgnore 172
ISRIgnore 173
ISRIgnore 174
ISRIgnore 175
ISRIgnore 176
ISRIgnore 177
ISRIgnore 178
ISRIgnore 179
ISRIgnore 180
ISRIgnore 181
ISRIgnore 182
ISRIgnore 183
ISRIgnore 184
ISRIgnore 185
ISRIgnore 186
ISRIgnore 187
ISRIgnore 188
ISRIgnore 189
ISRIgnore 190
ISRIgnore 191
ISRIgnore 192
ISRIgnore 193
ISRIgnore 194
ISRIgnore 195
ISRIgnore 196
ISRIgnore 197
ISRIgnore 198
ISRIgnore 199
ISRIgnore 200
ISRIgnore 201
ISRIgnore 202
ISRIgnore 203
ISRIgnore 204
ISRIgnore 205
ISRIgnore 206
ISRIgnore 207
ISRIgnore 208
ISRIgnore 209
ISRIgnore 210
ISRIgnore 211
ISRIgnore 212
ISRIgnore 213
ISRIgnore 214
ISRIgnore 215
ISRIgnore 216
ISRIgnore 217
ISRIgnore 218
ISRIgnore 219
ISRIgnore 220
ISRIgnore 221
ISRIgnore 222
ISRIgnore 223
ISRIgnore 224
ISRIgnore 225
ISRIgnore 226
ISRIgnore 227
ISRIgnore 228
ISRIgnore 229
ISRIgnore 230
ISRIgnore 231
ISRIgnore 232
ISRIgnore 233
ISRIgnore 234
ISRIgnore 235
ISRIgnore 236
ISRIgnore 237
ISRIgnore 238
ISRIgnore 239
ISRIgnore 240
ISRIgnore 241
ISRIgnore 242
ISRIgnore 243
ISRIgnore 244
ISRIgnore 245
ISRIgnore 246
ISRIgnore 247
ISRIgnore 248
ISRIgnore 249
ISRIgnore 250
ISRIgnore 251
ISRIgnore 252
ISRIgnore 253
ISRIgnore 254
ISRIgnore 255
