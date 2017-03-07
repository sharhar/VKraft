vkff = open("vulkan_funcs.txt")
vkfs = vkff.read()
vkff.close()

define = ""
head = ""
body = ""

for vkf in vkfs.split("\n"):
	if(not vkf == ""):
		define += "#define " + vkf + " g_p" + vkf + "\n"
		head += "PFN_" + vkf + " g_p" + vkf + ";\n"
		body += "g_p" + vkf + " = (PFN_" + vkf + ')glfwGetInstanceProcAddress(instance,"' + vkf + '");\n'


df = open("vexl_define.txt", "w")
df.write(define)
df.close()
		
hf = open("vexl_header.txt", "w")
hf.write(head)
hf.close()

cf = open("vexl_loader.txt", "w")
cf.write(body)
cf.close()
