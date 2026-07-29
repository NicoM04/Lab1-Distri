#pragma once

#include "./CudaCheck.cuh"

#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class CudaBuffer {
public:
    using value_type = T;

    CudaBuffer() noexcept = default;

    explicit CudaBuffer(std::size_t element_count) {
        allocate(element_count);
    }

    ~CudaBuffer() {
        release();
    }

    CudaBuffer(const CudaBuffer&) = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;

    CudaBuffer(CudaBuffer&& other) noexcept {
        moveFrom(std::move(other));
    }

    CudaBuffer& operator=(CudaBuffer&& other) noexcept {
        if (this != &other) {
            release();
            moveFrom(std::move(other));
        }

        return *this;
    }

    void allocate(std::size_t element_count) {
        release();

        if (element_count == 0U) {
            return;
        }

        element_count_ = element_count;
        CUDA_CHECK(cudaMalloc(
            reinterpret_cast<void**>(&device_ptr_),
            element_count_ * sizeof(T)
        ));
    }

    void copyToDevice(const T* host_ptr, std::size_t element_count) {
        validateTransfer(host_ptr, element_count);

        if (element_count == 0U) {
            return;
        }

        CUDA_CHECK(cudaMemcpy(
            device_ptr_,
            host_ptr,
            element_count * sizeof(T),
            cudaMemcpyHostToDevice
        ));

        ++host_to_device_copies_;
    }

    void copyToHost(T* host_ptr, std::size_t element_count) const {
        validateTransfer(host_ptr, element_count);

        if (element_count == 0U) {
            return;
        }

        CUDA_CHECK(cudaMemcpy(
            host_ptr,
            device_ptr_,
            element_count * sizeof(T),
            cudaMemcpyDeviceToHost
        ));

        ++device_to_host_copies_;
    }

    void copyDeviceToDevice(const CudaBuffer& source, std::size_t element_count) {
        validateTransfer(device_ptr_, element_count);
        source.validateTransfer(source.device_ptr_, element_count);

        if (element_count == 0U) {
            return;
        }

        CUDA_CHECK(cudaMemcpy(
            device_ptr_,
            source.device_ptr_,
            element_count * sizeof(T),
            cudaMemcpyDeviceToDevice
        ));

        ++device_to_device_copies_;
    }

    T* data() noexcept {
        return device_ptr_;
    }

    const T* data() const noexcept {
        return device_ptr_;
    }

    std::size_t size() const noexcept {
        return element_count_;
    }

    bool empty() const noexcept {
        return device_ptr_ == nullptr;
    }

    static void resetTransferStatistics() {
        host_to_device_copies_ = 0U;
        device_to_host_copies_ = 0U;
        device_to_device_copies_ = 0U;
    }

    static std::size_t hostToDeviceCopyCount() {
        return host_to_device_copies_;
    }

    static std::size_t deviceToHostCopyCount() {
        return device_to_host_copies_;
    }

    static std::size_t deviceToDeviceCopyCount() {
        return device_to_device_copies_;
    }

private:
    T* device_ptr_ = nullptr;
    std::size_t element_count_ = 0U;

    inline static std::size_t host_to_device_copies_ = 0U;
    inline static std::size_t device_to_host_copies_ = 0U;
    inline static std::size_t device_to_device_copies_ = 0U;

    void release() {
        if (device_ptr_ != nullptr) {
            CUDA_CHECK(cudaFree(device_ptr_));
            device_ptr_ = nullptr;
        }

        element_count_ = 0U;
    }

    void moveFrom(CudaBuffer&& other) noexcept {
        device_ptr_ = other.device_ptr_;
        element_count_ = other.element_count_;
        other.device_ptr_ = nullptr;
        other.element_count_ = 0U;
    }

    void validateTransfer(const T* ptr, std::size_t element_count) const {
        if (element_count > element_count_) {
            throw std::out_of_range("CudaBuffer transfer exceeds allocated size");
        }

        if (element_count > 0U && ptr == nullptr) {
            throw std::invalid_argument("CudaBuffer transfer received null pointer");
        }

        if (element_count > 0U && device_ptr_ == nullptr) {
            throw std::runtime_error("CudaBuffer has no allocated device storage");
        }
    }

    void validateTransfer(T* ptr, std::size_t element_count) const {
        validateTransfer(static_cast<const T*>(ptr), element_count);
    }
};