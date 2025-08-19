AP=ansible-playbook

launch: launch.yml
	$(AP) $<

provision: provision.yml
	$(AP) -i hosts $<

gen-deploy-key: gen-deploy-key.yml
	$(AP) -i hosts $<

install-scallop: install-scallop.yml
	$(AP) -i hosts $<

.PHONY: launch provision gen-deploy-key install-scallop