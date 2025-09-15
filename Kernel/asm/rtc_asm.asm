section .text

; lo agarra time.h
global _rtc_get_time
_rtc_get_time:
	mov rax, rdi	; recibimos por parámetro.
	out 70h, al		; 70h entrada para la informacion que quiero en 71h.
	mov rax, 0
	in al, 71h		; en al se deposita lo pedido por 70h, presente en 71h.
	ret
