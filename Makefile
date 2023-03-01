
install: requirements

requirements: requirements.txt
	@pip install -r requirements.txt

watch:
	@arduino-web-inject .

release:
	@git add .
	@git commit -am "Release"
	@git push
