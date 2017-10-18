.PHONY=slave host default clean .burn .reset


default:
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!build host image !!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@make -f kCamper.mk MODE=HOST


.burn:
	@make -f kCamper.mk MODE=HOST .burn
	@rm .burn

.reset:
	@make -f kCamper.mk .reset



clean:
	@make -f kCamper.mk clean
