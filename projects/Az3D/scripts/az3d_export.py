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
import math
from enum import Enum

def align(s):
	return ((s+3)//4)*4

def clamp(value, minimum, maximum):
	return max(minimum, min(maximum, value))

def padding(length):
	return b'\0' * (align(length) - length)

def write_padded(out, b):
	out.write(b)
	out.write(padding(len(b)))

def write_padded_buffer(buffer, b):
	buffer += b
	buffer += padding(len(b))

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
			return ((get_filepath_from_image(node_tex.image), (node_tex.image.size[0], node_tex.image.size[1])), [1, 1, 1, 1])
		else:
			color = get_default_from_node_input(node_input)
			assert(color)
			return (("", (0, 0)), [color[0], color[1], color[2], color[3]])
	else:
		color = node_input.default_value
		return (("", (0, 0)), [color[0], color[1], color[2], color[3]])

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
			return ((get_filepath_from_image(node_tex.image), (node_tex.image.size[0], node_tex.image.size[1])), factor)
	return (("", (0, 0)), 1)

def get_factor_from_node_input(node_input):
	if node_input.links:
		link = node_input.links[0]
		node_tex = follow_links_to_find_node(link, True, bpy.types.ShaderNodeTexImage)
		if node_tex:
			return ((get_filepath_from_image(node_tex.image), (node_tex.image.size[0], node_tex.image.size[1])), 1)
		else:
			factor = get_default_from_node_input(node_input)
			assert(factor)
			return (("", (0, 0)), factor)
	else:
		factor = node_input.default_value
		return (("", (0, 0)), factor)

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
				material["isFoliage"] = "#foliage" in mat.name
				materials.append(material)
	return materials

def get_material_max_tex_size(material):
	max_tex_size = 1
	def check(a):
		nonlocal max_tex_size
		if a[0] > max_tex_size:
			max_tex_size = a[0]
		if a[1] > max_tex_size:
			max_tex_size = a[1]
	check(material["albedoFile"][1])
	check(material["subsurfFile"][1])
	check(material["emissionFile"][1])
	check(material["normalFile"][1])
	check(material["metalnessFile"][1])
	check(material["roughnessFile"][1])
	return max_tex_size

versionMajor = 1
versionMinor = 1

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
	albedo    = get_tex_index(material["albedoFile"][0], False)
	subsurf   = get_tex_index(material["subsurfFile"][0], False)
	emission  = get_tex_index(material["emissionFile"][0], False)
	normal    = get_tex_index(material["normalFile"][0], True)
	metalness = get_tex_index(material["metalnessFile"][0], True)
	roughness = get_tex_index(material["roughnessFile"][0], True)

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
	if material["isFoliage"]:
		data += b'Fol\0'
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
	# n-gons where n > 4 break calc_tangents and calc_normals_split
	bmesh.ops.triangulate(bm, faces=[face for face in bm.faces if len(face.verts) > 4])
	bm.to_mesh(mesh)
	bm.free()
	if mesh.uv_layers:
		mesh.calc_tangents()
	else:
		mesh.calc_normals_split()

def get_error(val):
	return abs(val - round(val))

class ScalarKind(Enum):
	F32 = 0
	S16 = 1
	S8  = 2

class MeshData:
	def __init__(self, hasUVs, hasNormalMap, hasBones, armatureName):
		self.dedup = dict()
		self.vertices = bytearray()
		self.indices = []
		self.nextVertIndex = 0
		self.vertexStride = 0
		self.vertexFormat = bytearray()
		self.componentCount = 0
		self.indexStride = 4
		self.hasUVs = hasUVs
		self.hasNormalMap = hasNormalMap
		self.hasBones = hasBones
		self.armatureName = armatureName
		# stored positions and uvs for signed int types are scaled and offset
		self.posDimensions = [0.0, 0.0, 0.0]
		self.posOffset = [0.0, 0.0, 0.0]
		self.uvDimensions = [0.0, 0.0]
		self.uvOffset = [0.0, 0.0]
		# used to determine dimension and offset
		self.posMin = [ 100000000.0, 100000000.0, 100000000.0]
		self.posMax = [-100000000.0,-100000000.0,-100000000.0]
		self.uvMin = [ 100000000.0, 100000000.0]
		self.uvMax = [-100000000.0,-100000000.0]
		# used to determine whether lower precision is acceptable
		self.numVerts = 0
		self.posErrorS8 = 0.0
		self.posErrorS16 = 0.0
		self.uvErrorS8 = 0.0
		self.uvErrorS16 = 0.0
		self.posScalarKind = ScalarKind.F32
		self.uvScalarKind = ScalarKind.F32

	def update_pos_bounds(self, pos):
		for i in range(3):
			if pos[i] < self.posMin[i]:
				self.posMin[i] = pos[i]
			if pos[i] > self.posMax[i]:
				self.posMax[i] = pos[i]

	def update_uv_bounds(self, uv):
		uv[1] = 1.0 - uv[1]
		for i in range(2):
			if uv[i] < self.uvMin[i]:
				self.uvMin[i] = uv[i]
			if uv[i] > self.uvMax[i]:
				self.uvMax[i] = uv[i]

	def calc_dimensions(self, ):
		for i in range(3):
			self.posDimensions[i] = (self.posMax[i] - self.posMin[i]) / 2
			self.posOffset[i] = (self.posMax[i] + self.posMin[i]) / 2
		if self.hasUVs:
			for i in range(2):
				self.uvDimensions[i] = (self.uvMax[i] - self.uvMin[i]) / 2
				self.uvOffset[i] = (self.uvMax[i] + self.uvMin[i]) / 2
		print("posDimensions: " + str(self.posDimensions) + ", posOffset: " + str(self.posOffset) + "\nuvDimensions: " + str(self.uvDimensions) + ", uvOffset: " + str(self.uvOffset))

	def add_pos_error(self, pos):
		for i in range(3):
			# n is for normalized
			n = (pos[i] - self.posOffset[i]) / self.posDimensions[i]
			errorS8 = get_error(n * 127) / 127
			errorS16 = get_error(n * 32767) / 32767
			if errorS8 > self.posErrorS8:
				self.posErrorS8 = errorS8
			if errorS16 > self.posErrorS16:
				self.posErrorS16 = errorS16

	def add_uv_error(self, uv):
		uv[1] = 1.0 - uv[1]
		for i in range(2):
			# n is for normalized
			n = (uv[i] - self.uvOffset[i]) / self.uvDimensions[i]
			errorS8 = get_error(n * 127) / 127
			errorS16 = get_error(n * 32767) / 32767
			if errorS8 > self.uvErrorS8:
				self.uvErrorS8 = errorS8
			if errorS16 > self.uvErrorS16:
				self.uvErrorS16 = errorS16

	def calc_format(self, posMaxError, uvMaxError, props: bpy.types.OperatorProperties):
		self.vertexFormat = bytearray()
		self.componentCount = 0
		self.vertexStride = 0

		def add_component(tag: bytes, scalarKind: ScalarKind, scalarCount: int, dims, offsets):
			scalarSize = 1
			scalarTag = b'B'
			match scalarKind:
				case ScalarKind.S8:
					pass
				case ScalarKind.S16:
					scalarSize = 2
					scalarTag = b'S'
				case ScalarKind.F32:
					scalarSize = 4
					scalarTag = b'F'
			self.vertexStride += scalarSize * scalarCount
			self.vertexFormat += tag + scalarTag + struct.pack('b', scalarCount)
			self.componentCount += 1
			if scalarKind != ScalarKind.F32:
				assert(len(dims) == scalarCount and len(offsets) == scalarCount)
				for i in range(scalarCount):
					self.vertexFormat += struct.pack('<ff', dims[i], offsets[i])

		match props.position_format:
			case "AUTO":
				if self.posErrorS8 < posMaxError:
					self.posScalarKind = ScalarKind.S8
				elif self.posErrorS16 < posMaxError:
					self.posScalarKind = ScalarKind.S16
				else:
					self.posScalarKind = ScalarKind.F32
			case "BYTE":
				self.posScalarKind = ScalarKind.S8
			case "SHORT":
				self.posScalarKind = ScalarKind.S16
			case "FLOAT":
				self.posScalarKind = ScalarKind.F32
		add_component(b"Po", self.posScalarKind, 3, self.posDimensions, self.posOffset)
		add_component(b"No", ScalarKind.S8, 3, (1, 1, 1), (0, 0, 0))
		if self.hasUVs:
			if self.hasNormalMap:
				add_component(b"Ta", ScalarKind.S8, 3, (1, 1, 1), (0, 0, 0))
			match props.texture_coordinate_format:
				case "AUTO":
					if self.uvErrorS8 <= uvMaxError:
						self.uvScalarKind = ScalarKind.S8
					elif self.uvErrorS16 <= uvMaxError:
						self.uvScalarKind = ScalarKind.S16
					else:
						self.uvScalarKind = ScalarKind.F32
				case "BYTE":
					self.uvScalarKind = ScalarKind.S8
				case "SHORT":
					self.uvScalarKind = ScalarKind.S16
				case "FLOAT":
					self.uvScalarKind = ScalarKind.F32
			add_component(b"UV", self.uvScalarKind, 2, self.uvDimensions, self.uvOffset)
		if self.hasBones:
			# Bone Index (up to 4 bones per vertex)
			add_component(b"BI", ScalarKind.S8, 4, (127, 127, 127, 127), (128, 128, 128, 128))
			# Bone Weights (1 per bone index)
			add_component(b"BW", ScalarKind.S8, 4, (127, 127, 127, 127), (128, 128, 128, 128))
		print("posScalarKind: " + str(self.posScalarKind) + "\nuvScalarKind: " + str(self.uvScalarKind) + "\nformat: " + str(self.vertexFormat))

# Adds zeros to the end of this bytearray such that it aligns on a 4-byte boundary
def pad(data: bytearray):
	data += padding(len(data))

# tex_indices should be a dict that maps from texture filenames to [index, isLinear]
def write_mesh(context: bpy.types.Context, props: bpy.types.OperatorProperties, out, object: bpy.types.Object, tex_indices: dict[str, list[int | bool]]):
	object_eval = None
	armature = None
	for modifier in object.modifiers:
		if modifier.type == 'ARMATURE' and modifier.object and modifier.object.type == 'ARMATURE':
			armature = modifier.object.data
	# This maps from vertex group indices to bone indices, where -1 means the group is not a bone group
	group_to_bone_indices = None
	if armature is not None:
		group_to_bone_indices = [armature.bones.find(group.name) for group in object.vertex_groups]
		print(group_to_bone_indices)
	if props.apply_modifiers:
		object_armature_poses = []
		if armature is not None:
			if armature.pose_position != 'REST':
				object_armature_poses.append((armature, armature.pose_position))
				armature.pose_position = 'REST'
		# TODO: Use subsurface modifier (perhaps for LOD, but also maybe to save on file size since we can apply it on file load)
		depsgraph = context.evaluated_depsgraph_get()
		object_eval = object.evaluated_get(depsgraph)
		mesh: bpy.types.Mesh = object_eval.to_mesh()
		for armature_pose in object_armature_poses:
			armature_pose[0].pose_position = armature_pose[1]
	else:
		object_eval = object
		mesh: bpy.types.Mesh = object_eval.to_mesh()
	prepare_mesh(mesh, object_eval.matrix_world.copy())
	s = props.scale
	materials = get_material_props(object)
	hasUVs = bool(mesh.uv_layers)
	uv_layer = mesh.uv_layers.active
	mesh_datas = {}
	# first find bounds
	for face in mesh.polygons:
		material_index = face.material_index
		assert(material_index < len(materials))
		mesh_data = mesh_datas.setdefault(material_index, MeshData(hasUVs, materials[material_index]["normalFile"][0] != '', group_to_bone_indices != None, None if armature == None else armature.name))
		def check_vert(vertIndex):
			mesh_data.numVerts += 1
			vert = mesh.loops[vertIndex]
			pos = s*mesh.vertices[vert.vertex_index].co
			mesh_data.update_pos_bounds(pos)
			if mesh_data.hasUVs:
				vertUV = uv_layer.data[vertIndex].uv.copy()
				mesh_data.update_uv_bounds(vertUV)
		for vert in face.loop_indices:
			check_vert(vert)

	# calculate the dimensions and offsets of the meshes as they're needed to calculate error
	for mesh_data in mesh_datas.values():
		mesh_data.calc_dimensions()

	# next determine error of lower-precision data types
	for face in mesh.polygons:
		material_index = face.material_index
		mesh_data = mesh_datas[material_index]
		def check_vert(vertIndex):
			vert = mesh.loops[vertIndex]
			pos = s*mesh.vertices[vert.vertex_index].co
			mesh_data.add_pos_error(pos)
			if mesh_data.hasUVs:
				vertUV = uv_layer.data[vertIndex].uv.copy()
				mesh_data.add_uv_error(vertUV)
		for vert in face.loop_indices:
			check_vert(vert)

	# calculate the vertex format based on error bounds
	for i, mesh_data in mesh_datas.items():
		mesh_data.calc_format(0.001, 0.5 / get_material_max_tex_size(materials[i]), props)

	# pack the vertex data
	for face in mesh.polygons:
		material_index = face.material_index
		mesh_data = mesh_datas[material_index]
		posFmt = ''
		posTransform = None
		match mesh_data.posScalarKind:
			case ScalarKind.F32:
				posFmt = '<fff'
				posTransform = lambda a : a
			case ScalarKind.S16:
				posFmt = '<hhh'
				posTransform = lambda a : [round((a[i] - mesh_data.posOffset[i]) / mesh_data.posDimensions[i] * 32767) for i in range(3)]
			case ScalarKind.S8:
				posFmt = '<bbb'
				posTransform = lambda a : [round((a[i] - mesh_data.posOffset[i]) / mesh_data.posDimensions[i] * 127) for i in range(3)]
		uvFmt = ''
		uvTransform = None
		match mesh_data.uvScalarKind:
			case ScalarKind.F32:
				uvFmt = '<ff'
				uvTransform = lambda a : a
			case ScalarKind.S16:
				uvFmt = '<hh'
				uvTransform = lambda a : [round((a[i] - mesh_data.uvOffset[i]) / mesh_data.uvDimensions[i] * 32767) for i in range(2)]
			case ScalarKind.S8:
				uvFmt = '<bb'
				uvTransform = lambda a : [round((a[i] - mesh_data.uvOffset[i]) / mesh_data.uvDimensions[i] * 127) for i in range(2)]
		def add_vert(vertInfo):
			if vertInfo[0] >= mesh_data.nextVertIndex:
				vert = mesh.loops[vertInfo[1]]
				vertUV = uv_layer.data[vertInfo[1]].uv if hasUVs else None
				pos = s*mesh.vertices[vert.vertex_index].co
				normal = vert.normal
				tangent = vert.tangent
				vertex = b''

				posActual = posTransform(pos)
				vertex += struct.pack(posFmt, posActual[0], posActual[1], posActual[2])

				vertex += struct.pack('<bbb',
					round(normal.x * 127),
					round(normal.y * 127),
					round(normal.z * 127)
				)

				if mesh_data.hasUVs:
					if mesh_data.hasNormalMap:
						vertex += struct.pack('<bbb',
							round(tangent.x * 127),
							round(tangent.y * 127),
							round(tangent.z * 127)
						)

					uvActual = uvTransform([vertUV.x, 1-vertUV.y])
					vertex += struct.pack(uvFmt, uvActual[0], uvActual[1])

				if mesh_data.hasBones:
					weights = sorted(
						[(group_to_bone_indices[group.group], group.weight) for group in mesh.vertices[vert.vertex_index].groups if group_to_bone_indices[group.group] != -1],
						key = lambda group: group[1],
						reverse=True
					)
					weight_sum = 0
					for i in range(min(4, len(weights))):
						weight_sum += weights[i][1]
					for i in range(min(4, len(weights))):
						weights[i] = (weights[i][0], round(weights[i][1] / weight_sum * 255))
					weight_sum = 0
					for i in range(min(4, len(weights))):
						weight_sum += weights[i][1]
					# Since our rounding might round the wrong way too many times, just give any remainder to the bone with the biggest influence
					weights[0] = (weights[0][0], weights[0][1] + 255-weight_sum)
					for i in range(4):
						vertex += struct.pack('<b', weights[i][0] - 128 if i < len(weights) else 127)
					for i in range(4):
						vertex += struct.pack('<b', weights[i][1] - 128 if i < len(weights) else -128)

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

	for i, mesh_data in mesh_datas.items():
		data = bytearray()
		write_name(data, object.name)
		data += b'Vert' + struct.pack('<IHH', len(mesh_data.vertices) // mesh_data.vertexStride, mesh_data.vertexStride, mesh_data.componentCount) + mesh_data.vertexFormat
		data += mesh_data.vertices
		pad(data)

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
		pad(data)

		write_mat0(data, materials[i], tex_indices)
		if mesh_data.hasBones:
			data += b'Arm\0'
			write_name(data, mesh_data.armatureName)
		write_section_header(out, b'Mesh', len(data))
		write_padded(out, data)

		print("Mesh Part: " + str(len(mesh_data.vertices) // mesh_data.vertexStride) + " vertices and " + str(len(mesh_data.indices)) + " indices, material " + str(i))

def write_empty(props, out, object):
	data = bytearray()
	loc = object.location * props.scale
	vec = object.rotation_euler
	write_name(data, object.name)
	data += struct.pack('<ffffff', loc.x, loc.y, loc.z, vec.x, vec.y, vec.z)
	write_section_header(out, b'Empt', len(data))
	write_padded(out, data)

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

def pack_vector3(vector):
	return struct.pack('<fff', vector[0], vector[1], vector[2])

def bone_has_ik_info(bone: bpy.types.PoseBone):
	return bone.ik_stretch != 0 or bone.lock_ik_x or bone.lock_ik_y or bone.lock_ik_z or bone.use_ik_limit_x or bone.use_ik_limit_y or bone.use_ik_limit_z

# Arm0, the first version of the Armature table
def write_armature(context: bpy.types.Context, props: bpy.types.OperatorProperties, out, object: bpy.types.Object):
	data = bytearray()
	object_bones = object.pose.bones
	assert(len(object_bones) < 255) # We only support up to 255 indices, where 255 means none
	bones: dict[bpy.types.PoseBone, int] = {bone: i for i, bone in enumerate(object_bones.values())} # Now that's pythonic!
	bones[None] = 255
	# We have the hierarchy in a specific order, so we can use indices to point to parent bones. This is the order we'll be writing them to disc, and the indices are what we'll write.
	# print(str(bones))
	write_name(data, object.name)
	data += struct.pack('<I', len(object_bones)) # u32 number of bones
	for bone in bones.keys():
		if bone == None: continue
		write_name(data, bone.name) # name of bone
		ik_target = None
		ik_pole = None
		for constraint in bone.constraints:
			if constraint.type == 'IK' and constraint.target == object:
				ik_target = object_bones[constraint.subtarget]
				if constraint.pole_target == object:
					ik_pole = object_bones[constraint.pole_subtarget]
		bitflags = (
			  (0b00000001 if bone.bone.use_deform else 0)
			| (0b00000010 if bone.is_in_ik_chain else 0) # This specifically means IK will be used for animations.
			| (0b00000100 if bone_has_ik_info(bone) else 0) # This is used for general IK, whether that be from an animation, or procedural. Either way we want to store bone limitations.
		)
		data += struct.pack('<BBBB', bones[bone.parent], bones[ik_target], bones[ik_pole], bitflags) # u8 index of parent, index of IK target, index of IK Pole target, bitflags
		# 4x vec3 basis vectors and offset for rest pose
		mat = bone.matrix
		if bone.parent:
			mat = bone.parent.matrix.inverted() @ mat
		data += pack_vector3(mat.col[0])
		data += pack_vector3(mat.col[1])
		data += pack_vector3(mat.col[2])
		data += pack_vector3(mat.col[3] * props.scale)
		length = (bone.tail - bone.head).magnitude
		data += struct.pack('<f', length * props.scale)
		if bone_has_ik_info(bone):
			ik_data = bytearray()
			limits_flags = (
				  (0b00000001 if bone.ik_stretch != 0 else 0)
				| (0b00000010 if bone.lock_ik_x else 0)
				| (0b00000100 if bone.lock_ik_y else 0)
				| (0b00001000 if bone.lock_ik_z else 0)
				| (0b00010000 if bone.use_ik_limit_x else 0)
				| (0b00100000 if bone.use_ik_limit_y else 0)
				| (0b01000000 if bone.use_ik_limit_z else 0)
			)
			ik_data += struct.pack('<I', limits_flags)
			def pack_float_to_short(value: float, minimum: float, maximum: float):
				return struct.pack('<H', round((clamp(value, minimum, maximum) - minimum) / (maximum - minimum) * 0xffff))
			if bone.ik_stretch != 0:
				ik_data += pack_float_to_short(bone.ik_stretch, 0, 1)
			if not bone.lock_ik_x:
				ik_data += pack_float_to_short(bone.ik_stiffness_x, 0, 1)
			if not bone.lock_ik_y:
				ik_data += pack_float_to_short(bone.ik_stiffness_y, 0, 1)
			if not bone.lock_ik_z:
				ik_data += pack_float_to_short(bone.ik_stiffness_z, 0, 1)
			if bone.use_ik_limit_x:
				ik_data += pack_float_to_short(bone.ik_min_x / math.pi * 180, -180, 0)
				ik_data += pack_float_to_short(bone.ik_max_x / math.pi * 180, 0, 180)
			if bone.use_ik_limit_y:
				ik_data += pack_float_to_short(bone.ik_min_y / math.pi * 180, -180, 0)
				ik_data += pack_float_to_short(bone.ik_max_y / math.pi * 180, 0, 180)
			if bone.use_ik_limit_z:
				ik_data += pack_float_to_short(bone.ik_min_z / math.pi * 180, -180, 0)
				ik_data += pack_float_to_short(bone.ik_max_z / math.pi * 180, 0, 180)
			data += b"IK" + struct.pack('<H', len(ik_data) + 4)
			data += ik_data
			pad(data)

	write_section_header(out, b'Arm0', len(data))
	write_padded(out, data)

def is_curve_totally_null(curve: bpy.types.FCurve):
	keyframePrev = None
	for keyframe in curve.keyframe_points:
		# From some playing around I believe this covers every case accurately.
		if keyframe.co[1] != 0 or (keyframePrev is not None and keyframePrev.interpolation == 'BEZIER' and keyframe.handle_left[1] != 0) or (keyframe.interpolation == 'BEZIER' and keyframe.handle_right[1] != 0) or (keyframe.interpolation == 'ELASTIC' and keyframe.amplitude != 0):
			return False
		keyframePrev = keyframe
	return True

def keyframe_x_to_time(context: bpy.types.Context, action: bpy.types.Action, x):
	start_frame = action.curve_frame_range[0]
	fps = context.scene.render.fps
	return (x - start_frame) / fps

def is_bezier_linear(p1, c1, c2, p2, epsilon=0.001):
	def get_y(x):
		x = clamp((x - p1[0]) / (p2[0] - p1[0]), 0, 1)
		return p1[1] * (1 - x) + p2[1] * x
	def is_on_line(c):
		return abs(c[1] - get_y(c[0])) < epsilon
	return is_on_line(c1) and is_on_line(c2)

# Keyframes are laid out in binary as:
# b"KF"
# size: u16 (whole keyframe size including this header, not including padding)
# time: f32
# val: f32
# interpID: u32 (enum below)
# + Any additional data for the kind of interpolation. Logic below
def pack_keyframe(context: bpy.types.Context, props: bpy.types.OperatorProperties, action: bpy.types.Action, keyframe: bpy.types.Keyframe, keyframeNext: bpy.types.Keyframe | None, is_location: bool):
	data = bytearray()
	time = keyframe_x_to_time(context, action, keyframe.co[0])
	interpID = {key: i for i, key in enumerate(['CONSTANT', 'LINEAR', 'BEZIER', 'SINE', 'QUAD', 'CUBIC', 'QUART', 'QUINT', 'EXPO', 'CIRC', 'BACK', 'BOUNCE', 'ELASTIC'])}
	easingID = {key: i for i, key in enumerate(['EASE_IN', 'EASE_OUT', 'EASE_IN_OUT'])}
	interpolation = 'CONSTANT' if keyframeNext == None else keyframe.interpolation
	s = props.scale if is_location else 1

	if interpolation == 'BEZIER':
		# Maybe we're CONSTANT or LINEAR
		cp1 = keyframe.handle_right
		cp2 = keyframeNext.handle_left
		if keyframeNext.co[1] == keyframe.co[1] and cp1[1] == keyframe.co[1] and cp2[1] == keyframe.co[1]:
			# print("Turns out our Bezier was actually just a constant")
			interpolation = 'CONSTANT'
		elif is_bezier_linear(keyframe.co, cp1, cp2, keyframeNext.co):
			# print("Turns out our Bezier was actually just a line")
			interpolation = 'LINEAR'

	data += struct.pack('<ffI', time, keyframe.co[1] * s, interpID[interpolation])

	if interpolation == 'BEZIER':
		# We invert the logic here because we'd rather store control points than handles
		cp1 = keyframe.handle_right
		cp2 = keyframeNext.handle_left
		data += struct.pack('<ffff',
			keyframe_x_to_time(context, action, cp1[0]), cp1[1] * s,
			keyframe_x_to_time(context, action, cp2[0]), cp2[1] * s
		)
	elif interpID[interpolation] >= interpID['SINE']:
		easing = keyframe.easing
		if easing == 'AUTO':
			easing = 'EASE_IN' if interpID[interpolation] < interpID['BACK'] else 'EASE_OUT'
		data += struct.pack('<I', easingID[easing])
	if interpolation == 'BACK':
		data += struct.pack('<f', keyframe.back)
	elif interpolation == 'ELASTIC':
		data += struct.pack('<ff', keyframe.amplitude, keyframe.period / context.scene.render.fps)
	return b"KF" + struct.pack('<H', len(data) + 4) + data

# Act0, the first version of the Action table
def write_action(context: bpy.types.Context, props: bpy.types.OperatorProperties, out, action: bpy.types.Action):
	print("Exporting action \"" + action.name + "\" with " + str(len(action.fcurves)) + " curves")
	data = bytearray()
	write_name(data, action.name)
	curves = [curve for curve in action.fcurves if not curve.mute and not is_curve_totally_null(curve)]
	if len(curves) != len(action.fcurves):
		print("Actually there are only " + str(len(curves)) + " curves, since some are muted or do nothing")
	data += struct.pack('<I', len(curves))
	for curve in curves:
		name = curve.data_path + '[' + str(curve.array_index) + ']'
		# print(name)
		write_name(data, name)
		data += struct.pack('<I', len(curve.keyframe_points))
		is_cyclic = any(mod.type == 'CYCLES' for mod in curve.modifiers)
		is_location = curve.data_path.endswith("location")
		# if is_cyclic:
		# 	print(name + " is cyclic")
		for i, keyframe in enumerate(curve.keyframe_points):
			# print("keyframe co = " + str((keyframe_x_to_time(context, action, keyframe.co[0]), keyframe.co[1])))
			data += pack_keyframe(context, props, action, keyframe, curve.keyframe_points[i+1] if i+1 < len(curve.keyframe_points) else (curve.keyframe_points[0] if is_cyclic else None), is_location)
			data += padding(len(data))
	write_section_header(out, b'Act0', len(data))
	write_padded(out, data)


class ExportAZ3D(bpy.types.Operator, ExportHelper):
	"""Exports all relevant objects in the scene that define an AZ3D Object."""
	bl_idname = "export_scene.az3d"
	bl_label = "Export AZ3D Object (.az3d)"
	bl_options = {'REGISTER'}

	filename_ext = ".az3d"

	apply_modifiers: BoolProperty(name="Apply Modifiers", description="Applies the Modifiers", default=True)

	scale: FloatProperty(name="Scale", description="Scales all coordinates by a factor", default=1.0)

	position_format: EnumProperty(items=[
		("AUTO", "Auto", "Automatically choose data format based on error margins.", 1),
		("BYTE", "Byte", "Use signed 8-bit integers.", 2),
		("SHORT", "Short", "Use signed 16-bit integers.", 3),
		("FLOAT", "Float", "Use 32-bit floating point numbers.", 4),
	], description="Vertex Position Data Format", default="AUTO")

	texture_coordinate_format: EnumProperty(items=[
		("AUTO", "Auto", "Automatically choose data format based on error margins.", 1),
		("BYTE", "Byte", "Use signed 8-bit integers.", 2),
		("SHORT", "Short", "Use signed 16-bit integers.", 3),
		("FLOAT", "Float", "Use 32-bit floating point numbers.", 4),
	], description="Texture Coordinate Data Format", default="AUTO")


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
			elif obj.type == 'ARMATURE':
				write_armature(context, props, out, obj)
		for action in bpy.data.actions:
			if action.users == 0: continue # ignore orphans :(
			write_action(context, props, out, action)
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