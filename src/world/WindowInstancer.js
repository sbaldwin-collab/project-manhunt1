import * as THREE from 'three';

// Every lit/unlit apartment window in the city shares one draw call via
// InstancedMesh + per-instance color, instead of one PlaneGeometry mesh per
// window (which would be hundreds of draw calls across 4 blocks).
export class WindowInstancer {
  constructor(scene, maxCount = 900) {
    const geo = new THREE.PlaneGeometry(1.05, 0.78);
    const mat = new THREE.MeshBasicMaterial({
      vertexColors: true,
      toneMapped: false,
    });
    this.mesh = new THREE.InstancedMesh(geo, mat, maxCount);
    this.mesh.instanceColor = new THREE.InstancedBufferAttribute(
      new Float32Array(maxCount * 3),
      3,
    );
    this.mesh.count = 0;
    this.mesh.frustumCulled = false;
    scene.add(this.mesh);
    this._dummy = new THREE.Object3D();
    this._index = 0;
    this._max = maxCount;
  }

  add(x, y, z, rotY, lit, warm) {
    if (this._index >= this._max) return;
    this._dummy.position.set(x, y, z);
    this._dummy.rotation.set(0, rotY, 0);
    this._dummy.updateMatrix();
    this.mesh.setMatrixAt(this._index, this._dummy.matrix);
    const color = !lit
      ? new THREE.Color(0x1c2530)
      : warm
        ? new THREE.Color(0xffce87)
        : new THREE.Color(0xb9d6ff);
    this.mesh.setColorAt(this._index, color);
    this._index++;
    this.mesh.count = this._index;
  }

  finalize() {
    this.mesh.instanceMatrix.needsUpdate = true;
    if (this.mesh.instanceColor) this.mesh.instanceColor.needsUpdate = true;
  }
}
