private:
    AudioFormat format;
    
    // 用一个指针隐藏底层的解码器细节 (Pimpl idiom)
    struct PrivateData;
    PrivateData* pData;
    
    bool isInitialized;
