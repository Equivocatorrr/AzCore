bl_info = {
	"name": "Export AZ3D",
	"author": "Philip Haynes",
	"blender": (3, 4, 0),
	"location": "File > Export",
	"description": "Export to AZ3D Objects file",
	"category": "Import-Export",
}

import bpy
from bpy.props import *
from bpy_extras.io_utils import ExportHelper
import bmesh
import os
import struct

def align(s):
	return ((s+3)//4)*4

def write_padded(file, b):
	file.write(b)
	leftover = align(len(b)) - len(b)
	file.write(b'\0' * leftover)

def get_node_group_output_node(nodes):
	for node in nodes:
		if isinstance(node, bpy.types.NodeGroupOutput):
			return node
	return None

def get_node_input_from_output_internal_link(node, socket):
	for link in node.internal_links:
		if link.to_socket == socket:
			return link.from_socket
	return None

def traverse_links(link, internal, user_data, action, group_stack=[]):
	node = link.from_node
	socket = link.from_socket
	if isinstance(node, bpy.types.ShaderNodeGroup):
		node_link = get_node_group_output_node(link.from_node.node_tree.nodes).inputs[socket.name].links[0]
		group_stack.append(node)
		return traverse_links(node_link, internal, user_data, action, group_stack)
	elif isinstance(node, bpy.types.NodeGroupInput):
		assert(group_stack)
		group_socket = group_stack[-1].inputs[socket.name]
		if group_socket.links:
			group_stack.pop()
			return traverse_links(group_socket.links[0], internal, user_data, action, group_stack)
	val = action(link, internal, user_data, group_stack)
	if not val is None: return val
	if internal:
		node_input = get_node_input_from_output_internal_link(node, socket)
		for node_link in node_input.links:
			val = traverse_links(node_link, internal, user_data, action, group_stack)
			if not val is None:
				return val
	else:
		for node_input in node.inputs:
			for node_link in node_input.links:
				val = traverse_links(node_link, internal, user_data, action, group_stack)
				if not val is None:
					return val

def follow_links_to_find_node(link, internal, node_type, group_stack=[]):
	def action(link, internal, node_type, group_stack):
		node = link.from_node
		if isinstance(node, node_type):
			return node
	return traverse_links(link, internal, node_type, action, group_stack)

def follow_links_to_find_default(link, internal, user_data=None, group_stack=[]):
	def action(link, internal, user_data, group_stack):
		node = link.from_node
		socket = link.from_socket
		if isinstance(node, bpy.types.NodeGroupInput):
			assert(group_stack)
			group_socket = group_stack[-1].inputs[socket.name]
			if not group_socket.links:
				return group_socket.default_value
	return traverse_links(link, internal, user_data, action, group_stack)

def node_input_is_default_value(node, socket):
	node_input = get_node_input_from_output_internal_link(node, socket)
	if node_input:
		return not node_input.links

def get_default_from_node_input(node_input, group_stack=[]):
	if node_input.links:
		link = node_input.links[0]
		value = follow_links_to_find_default(link, True, None, group_stack)
		return value

def get_filepath_from_image(image):
	return os.path.normpath(bpy.path.abspath(image.filepath, library=image.library))

def get_color_from_node_input(node_input):
	if node_input.links:
		link = node_input.links[0]
		node_tex = follow_links_to_find_node(link, True, bpy.types.ShaderNodeTexImage)
		if node_tex:
			return (get_filepath_from_image(node_tex.image), [1, 1, 1, 1])
		else:
			color = get_default_from_node_input(node_input)
			assert(color)
			return ("", [color[0], color[1], color[2], color[3]])
	else:
		color = node_input.default_value
		return ("", [color[0], color[1], color[2], color[3]])

def get_normal_from_node_input(node_input):
	if node_input.links:
		link = node_input.links[0]
		group_stack=[]
		node_normal_map = follow_links_to_find_node(link, True, bpy.types.ShaderNodeNormalMap, group_stack)
		if node_normal_map:
			factor = get_default_from_node_input(node_normal_map.inputs[0], group_stack.copy())
			if factor is None:
				factor = 1
			node_tex = follow_links_to_find_node(node_normal_map.inputs[1].links[0], True, bpy.types.ShaderNodeTexImage, group_stack)
			return (get_filepath_from_image(node_tex.image), factor)
	return ("", 1)

def get_factor_from_node_input(node_input):
	if node_input.links:
		link = node_input.links[0]
		node_tex = follow_links_to_find_node(link, True, bpy.types.ShaderNodeTexImage)
		if node_tex:
			return (get_filepath_from_image(node_tex.image), 1)
		else:
			factor = get_default_from_node_input(node_input)
			assert(factor)
			return ("", factor)
	else:
		factor = node_input.default_value
		return ("", factor)

def get_material_props(obj):
	materials = []
	for mat in obj.data.materials:
		for node in mat.node_tree.nodes:
			if node.type == 'BSDF_PRINCIPLED':
				material = {}
				material["albedoFile"], material["albedoColor"] = get_color_from_node_input(node.inputs[0])
				material["emissionFile"], material["emissionColor"] = get_color_from_node_input(node.inputs[19])
				material["normalFile"], material["normalDepth"] = get_normal_from_node_input(node.inputs[22])
				material["emissionColor"].pop() # we don't want alpha
				material["metalnessFile"], material["metalnessFactor"] = get_factor_from_node_input(node.inputs[6])
				material["roughnessFile"], material["roughnessFactor"] = get_factor_from_node_input(node.inputs[9])
				materials.append(material)
	return materials

versionMajor = 1
versionMinor = 0
# how many bytes wide is a single element of a vertex array
vertexStride = 8*4
# how many bytes wide is a single element of an index array
indexStride = 4

def write_header(out):
	out.write(b'Az3DObj\0')
	out.write(struct.pack('<HHHH', versionMajor, versionMinor, vertexStride, indexStride))

# ident must be 4 bytes
# lengthInBytes doesn't include this header on the way in, but the written value does
def write_section_header(out, ident, lengthInBytes):
	assert len(ident) == 4
	out.write(ident)
	out.write(struct.pack('<I', lengthInBytes + 8))

def write_name(out, name):
	out.write(b'Name' + struct.pack('<I', len(name)))
	write_padded(out, bytes(name, 'utf-8'))

def get_name_byte_len(name):
	return 4*2 + align(len(bytes(name, 'utf-8')))

# mat0 is the first version of material data
def write_mat0(out, material, tex_indices):
	def get_tex_index(filename, linear):
		return tex_indices.setdefault(filename, [len(tex_indices), linear])[0]
	c = material["albedoColor"]
	e = material["emissionColor"]
	n = material["normalDepth"]
	m = material["metalnessFactor"]
	r = material["roughnessFactor"]
	albedo    = get_tex_index(material["albedoFile"], False)
	emission  = get_tex_index(material["emissionFile"], False)
	normal    = get_tex_index(material["normalFile"], True)
	metalness = get_tex_index(material["metalnessFile"], True)
	roughness = get_tex_index(material["roughnessFile"], True)
	print(material)
	out.write(b'Mat0' + struct.pack('<ffffffffffIIIII', c[0], c[1], c[2], c[3], e[0], e[1], e[2], n, m, r, albedo, emission, normal, metalness, roughness))

def prepare_mesh(mesh, transform):
	bm = bmesh.new()
	bm.from_mesh(mesh)
	# Remove location
	transform[0][3] = 0
	transform[1][3] = 0
	transform[2][3] = 0
	bm.transform(transform)
	bmesh.ops.triangulate(bm, faces=bm.faces[:])
	bm.to_mesh(mesh)
	bm.free()

# tex_indices should be a dict that maps from texture filenames to [index, isLinear]
def write_mesh(context, props, out, object, tex_indices):
	depsgraph = context.evaluated_depsgraph_get()
	object_eval = object.evaluated_get(depsgraph)
	mesh = object_eval.to_mesh()
	prepare_mesh(mesh, object_eval.matrix_world.copy())
	s = props.scale
	materials = get_material_props(object)
	uv_layer = mesh.uv_layers.active
	class MeshData:
		def __init__(self):
			self.dedup = dict()
			self.vertices = bytearray()
			self.indices = bytearray()
			self.nextVertIndex = 0
	mesh_datas = {}
	uvIndex = 0
	for face in mesh.polygons:
		material_index = face.material_index
		assert(material_index < len(materials))
		mesh_data = mesh_datas.setdefault(material_index, MeshData())
		normal = face.normal
		for index in face.vertices:
			vertUV = uv_layer.data[uvIndex].uv
			vert = mesh.vertices[index]
			if face.use_smooth:
				normal = vert.normal
			vertex = struct.pack('<ffffffff', s*vert.co.x, s*vert.co.y, s*vert.co.z, normal.x, normal.y, normal.z, vertUV.x, 1-vertUV.y)
			vertIndex = mesh_data.dedup.setdefault(vertex, mesh_data.nextVertIndex)
			if vertIndex == mesh_data.nextVertIndex:
				mesh_data.vertices += vertex
				mesh_data.nextVertIndex += 1
			mesh_data.indices += struct.pack('<I', vertIndex)
			uvIndex += 1
	
	# figure out how big we are
	total_bytes = get_name_byte_len(object.name)
	for mesh_data in mesh_datas.values():
		# Vert and Indx headers and data
		total_bytes += 4*4 + len(mesh_data.vertices) + len(mesh_data.indices)
		# Mat0 data
		total_bytes += 4*16
	
	# do the writing
	write_section_header(out, b'Mesh', total_bytes)
	write_name(out, object.name)
	
	for i, mesh_data in mesh_datas.items():
		out.write(b'Vert' + struct.pack('<I', len(mesh_data.vertices) // vertexStride))
		out.write(mesh_data.vertices)
		
		out.write(b'Indx' + struct.pack('<I', len(mesh_data.indices) // indexStride))
		out.write(mesh_data.indices)
		
		write_mat0(out, materials[i], tex_indices)
		
		print("Mesh Part: " + str(len(mesh_data.vertices) // vertexStride) + " vertices and " + str(len(mesh_data.indices) // indexStride) + " indices, material " + str(i))

def write_empty(props, out, object):
	total_bytes = get_name_byte_len(object.name) + 4*6
	write_section_header(out, b'Empt', total_bytes)
	write_name(out, object.name)
	loc = object.location * props.scale
	vec = object.rotation_euler
	out.write(struct.pack('<ffffff', loc.x, loc.y, loc.z, vec.x, vec.y, vec.z))

def write_textures(out, tex_indices):
	class FileInfo:
		def __init__(self):
			self.linear = False
			self.name = ''
			self.data = b''
	
	file_infos = []
	total_bytes = 0
	for filepath, info in tex_indices.items():
		if filepath == '': continue
		file_info = FileInfo()
		file_info.linear = info[1]
		file_info.name = os.path.basename(filepath)
		file = open(filepath, 'rb')
		file_info.data = file.read()
		file.close()
		total_bytes += get_name_byte_len(file_info.name) + 4*2 + align(len(file_info.data))
		file_infos.append(file_info)
	if total_bytes == 0: return
	total_bytes += 4
	write_section_header(out, b'Imgs', total_bytes)
	out.write(struct.pack('<I', len(file_infos)))
	for file_info in file_infos:
		write_name(out, file_info.name)
		assert(len(file_info.data) <= 0xffffffff)
		out.write(struct.pack('<II', len(file_info.data), file_info.linear))
		write_padded(out, file_info.data)

class ExportAZ3D(bpy.types.Operator, ExportHelper):
	"""Exports all relevant objects in the scene that define an AZ3D Object."""
	bl_idname = "export_scene.az3d"
	bl_label = "Export AZ3D Object (.az3d)"
	bl_options = {'REGISTER'}
	
	filename_ext = ".az3d"
	
	apply_modifiers: BoolProperty(name="Apply Modifiers", description="Applies the Modifiers", default=True)
	
	scale: FloatProperty(name="Scale", description="Scales all coordinates by a factor", default=1.0)
	
	
	def execute(self, context):
		scene = context.scene
		props = self.properties
		filepath = self.filepath
		filepath = bpy.path.ensure_ext(filepath, self.filename_ext)
		out = open(filepath, 'wb')
		tex_indices = {"": [0, False]}
		write_header(out)
		for obj in scene.objects:
			if obj.hide_get(): continue
			if obj.type == 'MESH':
				write_mesh(context, props, out, obj, tex_indices)
			elif obj.type == 'EMPTY':
				write_empty(props, out, obj)
		write_textures(out, tex_indices)
		out.close()
			
		return {'FINISHED'}

def menu_func(self, context):
	self.layout.operator(ExportAZ3D.bl_idname, text="AZ3D Object (.az3d)")
	
def register():
	bpy.utils.register_class(ExportAZ3D)
	
	bpy.types.TOPBAR_MT_file_export.append(menu_func)
	
def unregister():
	bpy.utils.unregister_class(ExportAZ3D)
	
	bpy.types.TOPBAR_MT_file_export.remove(menu_func)
	
if __name__ == "__main__":
	register()