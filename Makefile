
install: requirements

requirements: requirements.txt
	@pip install -r requirements.txt

inject: export GLOBAL_VARS = appTitle="ESP8266 WiFi";hostname="esp8266.localhost.net"

inject: export CONFIG_APP_JS = $(shell python3 -m jsmin app/config/app.js | make -s escape)
inject: export CONFIG_INDEX_HTML = $(shell htmlmin -s app/config/index.html | make -s escape)
inject: export CONFIG_FORM_HTML = $(shell htmlmin -s app/config/form.html | make -s escape)

inject: export FRAMEWORK_JS = $(shell python3 -m jsmin app/framework.js | make -s escape)
inject: export STYLE_CSS = $(shell python3 -m csscompressor app/style.css | make -s escape)
inject: export WELCOME_HTML = $(shell htmlmin -s app/welcome.html | make -s escape)

inject:
	@cat app/main.cpp | sed \
		-e 's/String configIndexHtml =.*$$/String configIndexHtml = "$$${A}{CONFIG_INDEX_HTML}";/g' \
		-e 's/String configFormHtml =.*$$/String configFormHtml = "$$${A}{CONFIG_FORM_HTML}";/g' \
		-e 's/String welcomeHtml =.*$$/String welcomeHtml = "$$${A}{WELCOME_HTML}";/g' > app/main.cpp.tmp
	@envsubst < app/main.cpp.tmp | envsubst > src/main.cpp
	@rm app/main.cpp.tmp

escape:
	@sed -e 's/"/\\"/g' -e 's/<%/"+/g' -e 's/%>/+"/g'

build: inject

release:
	@git add .
	@git commit -am "Release"
	@git push
