	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.hrt_tx,"ax",@progbits
	.align	1
	.type	hrt_tx, @function
hrt_tx:
	lw	a5,4(a0)
	addi	sp,sp,-16
	sw	s0,12(sp)
	sw	s1,8(sp)
	sw	a2,0(sp)
	beq	a5,zero,.L1
	li	a4,32
	# WS2812: frame_width is always 1, so no division needed
	lui	t2,%hi(xfer_shift_ctrl)
	addi	a2,t2,%lo(xfer_shift_ctrl)
	lbu	s1,2(a2)
	lbu	a5,1(a2)
	sb	a1,2(a2)
	lbu	a2,3(a2)
	li	t1,3145728
	slli	a5,a5,8
	slli	a2,a2,20
	and	a2,a2,t1
	andi	a5,a5,1792
	or	a5,a5,a2
	li	t1,126976
	slli	a2,a1,12
	and	a2,a2,t1
	or	a5,a5,a2
	addi	a4,a4,-1
	andi	a4,a4,0xff
	sb	a4,%lo(xfer_shift_ctrl)(t2)
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	li	a4,2031616
	slli	a5,a1,16
	and	a5,a5,a4
	ori	a5,a5,4
	sw	a5,4(sp)
	li	t1,0
	addi	t0,t2,%lo(xfer_shift_ctrl)
.L3:
	lw	a5,4(a0)
	bltu	t1,a5,.L6
.L1:
	lw	s0,12(sp)
	lw	s1,8(sp)
	addi	sp,sp,16
	jr	ra
.L6:
	# WS2812: Simple word processing - no last/penultimate word complexity
	lw	a4,0(a0)
	li	a5,0
	beq	a4,zero,.L7
	lw	a5,0(a0)
	slli	a4,t1,2
	add	a5,a5,a4
	lw	a5,0(a5)
.L7:
	beq	a1,s1,.L8
.L9:
 #APP
	csrr a4, 3022
 #NO_APP
	andi	a4,a4,0xff
	bne	a4,zero,.L9
	lw	a4,4(sp)
 #APP
	csrw 3043, a4
 #NO_APP
	mv	s1,a1
.L8:
	lbu	a4,16(a0)
	andi	a2,a4,0xff
	beq	a4,zero,.L10
	li	a4,1
	bne	a2,a4,.L11
 #APP
	csrw 3017, a5
 #NO_APP
.L11:
	bne	t1,zero,.L12
	lw	a5,0(sp)
	lbu	a5,0(a5)
	bne	a5,zero,.L12
	mv	a5,a3
	bne	a3,zero,.L13
	li	a5,1
.L13:
	slli	a3,a5,16
	srli	a3,a3,16
 #APP
	csrw 2005, a3
 #NO_APP
	lw	a5,0(sp)
	li	a4,1
	sb	a4,0(a5)
.L12:
	addi	t1,t1,1
	j	.L3
.L10:
 #APP
	csrw 3016, a5
 #NO_APP
	j	.L11
	.size	hrt_tx, .-hrt_tx
	.section	.text.hrt_write,"ax",@progbits
	.align	1
	.globl	hrt_write
	.type	hrt_write, @function
hrt_write:
	addi	sp,sp,-16
	sw	s0,8(sp)
	sw	ra,12(sp)
	sw	s1,4(sp)
	mv	s0,a0
	sb	zero,3(sp)
	lhu	a5,90(a0)
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,0
	li	a2,20
	li	a3,4
.L21:
	mul	a4,a5,a2
	add	a4,s0,a4
	lw	a4,4(a4)
	bne	a4,zero,.L20
	addi	a5,a5,1
	bne	a5,a3,.L21
	li	a5,3
.L22:
	li	a4,1
	beq	a5,a4,.L23
	li	a4,3
	beq	a5,a4,.L24
	li	a3,0
	bne	a5,zero,.L25
	lbu	a3,80(s0)
.L43:
	andi	a3,a3,0xff
.L25:
	lui	a4,%hi(xfer_shift_ctrl+2)
	sb	a3,%lo(xfer_shift_ctrl+2)(a4)
 #APP
	csrw 2000, 2
 #NO_APP
	lhu	a4,84(s0)
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrr a2, 2003
 #NO_APP
	li	a1,-65536
	and	a2,a2,a1
	or	a4,a4,a2
 #APP
	csrw 2003, a4
	csrw 3011, 0
 #NO_APP
	li	a2,2031616
	slli	a4,a3,16
	and	a4,a4,a2
	ori	a4,a4,4
 #APP
	csrw 3043, a4
 #NO_APP
	li	a4,20
	mul	a5,a5,a4
	li	a2,1
	add	a5,s0,a5
	lw	a4,4(a5)
	beq	a4,a2,.L26
	li	a2,2
	beq	a4,a2,.L27
	li	a5,32
	div	a5,a5,a3
	j	.L44
.L20:
	andi	a5,a5,0xff
	j	.L22
.L23:
	lbu	a3,81(s0)
	j	.L43
.L24:
	lbu	a3,83(s0)
	j	.L43
.L26:
	lbu	a5,8(a5)
.L44:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L30
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
.L31:
 #APP
	csrr s1, 3008
 #NO_APP
	# WS2812 only needs data phase - single call to hrt_tx
	lbu	a1,83(s0)		# Load data bus width
	lhu	a3,84(s0)		# Load counter value  
	addi	a2,sp,3		# Counter running flag
	addi	a0,s0,60		# Data phase offset (HRT_FE_DATA)
	call	hrt_tx		# Single data transmission
	lbu	a5,94(s0)
	bne	a5,zero,.L32
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	li	a4,131072
	slli	a5,s1,1
	addi	a4,a4,-2
	and	a5,a5,a4
 #APP
	csrw 3012, a5
	csrw 2000, 0
 #NO_APP
.L33:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L19
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L36
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
.L19:
	lw	ra,12(sp)
	lw	s0,8(sp)
	lw	s1,4(sp)
	addi	sp,sp,16
	jr	ra
.L27:
	lbu	a5,9(a5)
	j	.L44
.L30:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L31
.L32:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L32
 #APP
	csrw 2000, 0
 #NO_APP
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	lbu	a4,94(s0)
	li	a5,1
	bne	a4,a5,.L34
 #APP
	csrc 3008, 1
 #NO_APP
	j	.L33
.L34:
	lbu	a4,94(s0)
	li	a5,3
	bne	a4,a5,.L33
 #APP
	csrs 3008, 1
 #NO_APP
	j	.L33
.L36:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L19
	.size	hrt_write, .-hrt_write
