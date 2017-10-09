.PHONY=slave host default clean .burn .reset
default:host slave

.burn:
	@make -f kCamper.mk MODE=HOST .burn
	@rm .burn

.reset:
	@make -f kCamper.mk .reset


slave:
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!build slave image!!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@make -f kCamper.mk MODE=SLAVE

host:
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!build host image !!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@make -f kCamper.mk MODE=HOST


clean:
	@make -f kCamper.mk clean
