vkff = open("vulkan_funcs.txt")
vkfs = vkff.read()
vkff.close()

head = ""
body = ""

for vkf in vkfs.split("\n"):
	if(not vkf == ""):
		head += "PFN_" + vkf + " g_p" + vkf + ";\n"
		body += "g_p" + vkf + " = (PFN_" + vkf + ')glfwGetInstanceProcAddress(instance,"' + vkf + '");\n'


hf = open("vexl_header.txt", "w")
hf.write(head)
hf.close()

cf = open("vexl_loader.txt", "w")
cf.write(body)
cf.close()
