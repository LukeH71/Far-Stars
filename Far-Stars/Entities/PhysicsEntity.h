
class PhysicsEntity {
public:
    
    PhysicsEntity (const vector_float4& color,
                   const vector_float3& position,
                   const vector_float3& scale,
                   const vector_float3& rotation,
                   const short& meshID )
        : color(color),
          position(position),
          scale(scale),
          rotation(rotation),
          meshID(meshID) 
    {
        
    }
    
    
    vector_float4 getColor () { return color; }
    vector_float3 getPosition () { return position; }
    vector_float3 getScale () { return scale; }
    vector_float3 getRotation () { return rotation; }

    short getMeshID () { return meshID; }
    
private:
    
    // Oreantation of the object in the world
    vector_float4 color;
    vector_float3 position;
    vector_float3 scale;
    vector_float3 rotation;
    
    // Movement of the object
    
    // Shape of the object
    short meshID;
};
