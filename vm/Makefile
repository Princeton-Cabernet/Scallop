AP=ansible-playbook

launch: launch.yml
	$(AP) $<

provision: provision.yml
	$(AP) -i hosts $<

.PHONY: launch provision