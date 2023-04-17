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

def write_padded(out, b):
	out.write(b)
	leftover = align(len(b)) - len(b)
	out.write(b'\0' * leftover)

def write_padded_buffer(buffer, b):
	buffer += b
	leftover = align(len(b)) - len(b)
	buffer += b'\0' * leftover

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
	else:
		return node_input.default_value

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
				material["subsurfFactor"] = get_default_from_node_input(node.inputs[1])
				material["subsurfRadius"] = get_default_from_node_input(node.inputs[2])
				material["subsurfFile"], material["subsurfColor"] = get_color_from_node_input(node.inputs[3])
				material["emissionFile"], material["emissionColor"] = get_color_from_node_input(node.inputs[19])
				material["normalFile"], material["normalDepth"] = get_normal_from_node_input(node.inputs[22])
				material["emissionColor"].pop() # we don't want alpha
				material["metalnessFile"], material["metalnessFactor"] = get_factor_from_node_input(node.inputs[6])
				material["roughnessFile"], material["roughnessFactor"] = get_factor_from_node_input(node.inputs[9])
				materials.append(material)
	return materials

versionMajor = 1
versionMinor = 0

def write_header(out):
	out.write(b'Az3DObj\0')
	out.write(struct.pack('<HH', versionMajor, versionMinor))

# ident must be 4 bytes
# lengthInBytes doesn't include this header on the way in, but the written value does
def write_section_header(out, ident, lengthInBytes):
	assert len(ident) == 4
	out.write(ident)
	out.write(struct.pack('<I', lengthInBytes + 8))

def write_name(buffer, name):
	buffer += b'Name' + struct.pack('<I', len(name))
	write_padded_buffer(buffer, bytes(name, 'utf-8'))

def get_name_byte_len(name):
	return 4*2 + align(len(bytes(name, 'utf-8')))

# mat0 is the first version of material data
def write_mat0(buffer, material, tex_indices):
	def get_tex_index(filename, linear):
		return tex_indices.setdefault(filename, [len(tex_indices), linear])[0]
	# Format tags have 2 chars for identification and 2 chars for type.
	# The first char in a type is the base type and the second is the count.
	# `F` means Float32, `I` means uint32
	# Example: ACF4 stands for Albedo Color Float x 4
	a = material["albedoColor"]
	sssF = material["subsurfFactor"]
	sssR = material["subsurfRadius"]
	sssC = material["subsurfColor"]
	e = material["emissionColor"]
	n = material["normalDepth"]
	m = material["metalnessFactor"]
	r = material["roughnessFactor"]
	albedo    = get_tex_index(material["albedoFile"], False)
	subsurf   = get_tex_index(material["subsurfFile"], False)
	emission  = get_tex_index(material["emissionFile"], False)
	normal    = get_tex_index(material["normalFile"], True)
	metalness = get_tex_index(material["metalnessFile"], True)
	roughness = get_tex_index(material["roughnessFile"], True)
	
	data = bytearray()
	# A_ is for Albedo
	# E_ is for Emission
	# N_ is for Normal
	# M_ is for Metalness
	# R_ is for Roughness
	# S_ is for Subsurface Scattering
	# _C is for Color
	# _F is for Factor
	# _T is for Texture
	# _R is for Radius
	data += b'ACF\004' + struct.pack('<ffff', a[0], a[1], a[2], a[3])
	data += b'ECF\003' + struct.pack('<fff', e[0], e[1], e[2])
	data += b'MFF\001' + struct.pack('<f', m)
	data += b'RFF\001' + struct.pack('<f', r)
	if albedo != 0:
		data += b'ATI\001' + struct.pack('<I', albedo)
	if emission != 0:
		data += b'ETI\001' + struct.pack('<I', emission)
	if normal != 0:
		data += b'NDF\001' + struct.pack('<f', n)
		data += b'NTI\001' + struct.pack('<I', normal)
	if metalness != 0:
		data += b'MTI\001' + struct.pack('<I', metalness)
	if roughness != 0:
		data += b'RTI\001' + struct.pack('<I', roughness)
	if sssF != 0:
		data += b'SFF\001' + struct.pack('<f', sssF)
		if sssC != [1, 1, 1]:
			data += b'SCF\003' + struct.pack('<fff', sssC[0], sssC[1], sssC[2])
		data += b'SRF\003' + struct.pack('<fff', sssR[0], sssR[1], sssR[2])
		if subsurf != 0:
			data += b'STI\001' + struct.pack('<I', subsurf)
	print(material)
	buffer += b'Mat0' + struct.pack('<I', len(data))
	buffer += data

def prepare_mesh(mesh, transform):
	bm = bmesh.new()
	bm.from_mesh(mesh)
	# Remove location
	transform[0][3] = 0
	transform[1][3] = 0
	transform[2][3] = 0
	bm.transform(transform)
	bm.to_mesh(mesh)
	bm.free()
	if mesh.uv_layers:
		mesh.calc_tangents()
	else:
		mesh.calc_normals_split()

class MeshData:
	def __init__(self, hasUVs, hasNormalMap):
		self.dedup = dict()
		self.vertices = bytearray()
		self.indices = []
		self.nextVertIndex = 0
		self.indexStride = 4
		if not hasUVs:
			self.vertexStride = 6*4
			self.vertexFormat = b'PoF\003NoF\003'
		else:
			if hasNormalMap:
				self.vertexStride = 11*4
				self.vertexFormat = b'PoF\003NoF\003TaF\003UVF\002'
			else:
				self.vertexStride = 8*4
				self.vertexFormat = b'PoF\003NoF\003UVF\002'

# tex_indices should be a dict that maps from texture filenames to [index, isLinear]
def write_mesh(context, props, out, object, tex_indices):
	depsgraph = context.evaluated_depsgraph_get()
	object_eval = object.evaluated_get(depsgraph)
	mesh = object_eval.to_mesh()
	prepare_mesh(mesh, object_eval.matrix_world.copy())
	s = props.scale
	materials = get_material_props(object)
	hasUVs = bool(mesh.uv_layers)
	uv_layer = mesh.uv_layers.active
	mesh_datas = {}
	for face in mesh.polygons:
		material_index = face.material_index
		assert(material_index < len(materials))
		mesh_data = mesh_datas.setdefault(material_index, MeshData(hasUVs, materials[material_index]["normalFile"] != ''))
		def add_vert(vertInfo):
			if vertInfo[0] >= mesh_data.nextVertIndex:
				vert = mesh.loops[vertInfo[1]]
				vertUV = uv_layer.data[vertInfo[1]].uv if hasUVs else None
				pos = s*mesh.vertices[vert.vertex_index].co
				normal = vert.normal
				tangent = vert.tangent
				match mesh_data.vertexStride:
					case 24:
						vertex = struct.pack('<ffffff', pos.x, pos.y, pos.z, normal.x, normal.y, normal.z)
					case 32:
						vertex = struct.pack('<ffffffff', pos.x, pos.y, pos.z, normal.x, normal.y, normal.z, vertUV.x, 1-vertUV.y)
					case 44:
						vertex = struct.pack('<fffffffffff', pos.x, pos.y, pos.z, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, vertUV.x, 1-vertUV.y)
				vertInfo[0] = mesh_data.dedup.setdefault(vertex, mesh_data.nextVertIndex)
				if vertInfo[0] == mesh_data.nextVertIndex:
					mesh_data.vertices += vertex
					mesh_data.nextVertIndex += 1
			mesh_data.indices.append(vertInfo[0])
		v1 = [mesh_data.nextVertIndex, face.loop_indices[0]]
		vPrev = [mesh_data.nextVertIndex+1,face.loop_indices[1]]
		for vert in face.loop_indices[2:]:
			add_vert(v1)
			add_vert(vPrev)
			newVert = [mesh_data.nextVertIndex, vert]
			add_vert(newVert)
			vPrev = newVert
	
	for mesh_data in mesh_datas.values():
		if len(mesh_data.indices) < 256:
			mesh_data.indexStride = 1
		elif len(mesh_data.indices) < 65536:
			mesh_data.indexStride = 2
	
	data = bytearray()
	write_name(data, object.name)
	
	for i, mesh_data in mesh_datas.items():
		data += b'Vert' + struct.pack('<IHH', len(mesh_data.vertices) // mesh_data.vertexStride, mesh_data.vertexStride, len(mesh_data.vertexFormat) // 4) + mesh_data.vertexFormat
		data += mesh_data.vertices
		
		# indexStride isn't explicitly written, but instead depends on how many vertices we have
		data += b'Indx' + struct.pack('<I', len(mesh_data.indices))
		fmt = ''
		match mesh_data.indexStride:
			case 1:
				fmt = '<B'
			case 2:
				fmt = '<H'
			case 4:
				fmt = '<I'
		for index in mesh_data.indices:
			data += struct.pack(fmt, index)
		data += b'\0' * (align(len(data)) - len(data))
		
		write_mat0(data, materials[i], tex_indices)
		
		print("Mesh Part: " + str(len(mesh_data.vertices) // mesh_data.vertexStride) + " vertices and " + str(len(mesh_data.indices)) + " indices, material " + str(i))
	
	# do the writing
	write_section_header(out, b'Mesh', len(data))
	out.write(data)

def write_empty(props, out, object):
	data = bytearray()
	loc = object.location * props.scale
	vec = object.rotation_euler
	write_name(data, object.name)
	data += struct.pack('<ffffff', loc.x, loc.y, loc.z, vec.x, vec.y, vec.z)
	write_section_header(out, b'Empt', len(data))
	out.write(data)

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
		name = bytearray()
		write_name(name, file_info.name)
		out.write(name)
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