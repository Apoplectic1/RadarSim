# Future Optimization Patterns

These are optimization patterns that could be implemented to improve performance. Currently not implemented.

## 1. Async Counter Query

Avoid GPU stall when reading hit count:

```cpp
// Instead of blocking glMapBufferRange:
glGetQueryObjectui64v(queryId, GL_QUERY_RESULT_NO_WAIT, &hitCount);
```

## 2. Double-Buffered Results

Pipeline GPU/CPU work by reading previous frame's results:

```cpp
// Frame N: Read buffer A (from frame N-1), Write buffer B
// Frame N+1: Read buffer B, Write buffer A
int readBuffer = frameCount % 2;
int writeBuffer = (frameCount + 1) % 2;
```

## 3. Persistent Mapped Buffers

Zero-copy readback using persistent mapping:

```cpp
glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr,
    GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
void* mapped = glMapBufferRange(..., GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
// Read directly from mapped pointer after fence sync
```

## 4. Fence Sync

Deferred readback without blocking:

```cpp
GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
// ... continue CPU work ...
GLenum result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
    // Safe to read results
}
```

## 5. Background Thread for BVH

Move CPU-intensive BVH construction off main thread:

```cpp
class BVHWorker : public QObject {
    Q_OBJECT
public slots:
    void buildBVH(const std::vector<float>& vertices,
                   const std::vector<uint32_t>& indices,
                   const QMatrix4x4& transform) {
        BVHBuilder builder;
        builder.build(vertices, indices, transform);
        emit bvhReady(builder.getNodes(), builder.getTriangles());
    }
signals:
    void bvhReady(std::vector<BVHNode> nodes, std::vector<Triangle> tris);
};

// In main thread:
QThread* bvhThread = new QThread();
BVHWorker* worker = new BVHWorker();
worker->moveToThread(bvhThread);
connect(this, &RCSCompute::buildRequested, worker, &BVHWorker::buildBVH);
connect(worker, &BVHWorker::bvhReady, this, &RCSCompute::onBVHReady);
```

## Guidelines for Adding Heavy Computation

**DO:**
- Keep OpenGL calls on main thread (Qt requirement)
- Use `glMemoryBarrier()` between dependent compute dispatches
- Use lazy evaluation (`dirty_` flags) to avoid unnecessary work
- Keep GPU-only data resident (like shadow map)
- Consider double-buffering for results needed every frame

**DON'T:**
- Call `glMapBufferRange()` with `GL_MAP_READ_BIT` in hot path (causes stall)
- Build BVH on main thread for large geometry (blocks UI)
- Read back data that can stay on GPU
- Dispatch compute without memory barriers between dependent stages

**Thread Safety:**
- `QOpenGLContext` must be used from thread that created it
- Use `QOffscreenSurface` + `makeCurrent()` for background GL threads
- Prefer signal/slot for cross-thread communication (Qt handles queuing)
